#!/usr/bin/env python3
"""Independent verification of results/benchmark-40-final-consolidated-v2.

Recomputes from raw sources (gate1_results.json, inventory.json, raw/lizard.csv,
raw/cppcheck_*.xml) without using consolidate.py or summary.json, and checks
the statistics against SciPy. Exit code 1 on any mismatch."""
import csv
import json
import math
import re
import sys
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path

from scipy.stats import binomtest

R = Path(__file__).resolve().parent / "results"
V2 = R / "benchmark-40-final-consolidated-v2"
problems = []


def check(name, got, want, tol=1e-6):
    ok = (got == want) if isinstance(want, (str, bool)) or want is None else (got is not None and abs(float(got) - float(want)) <= tol)
    if not ok:
        problems.append(f"{name}: got {got!r}, expected {want!r}")


inv = {r["combo_index"]: r for r in json.loads((R / "benchmark-40-final-gate1/inventory.json").read_text(encoding="utf-8"))["rows"]}
g1 = {r["combo_index"]: r["gate1_status"] for r in json.loads((R / "benchmark-40-final-gate1/gate1_results.json").read_text(encoding="utf-8"))}
models = sorted({r["model_label"] for r in inv.values()})

# --- raw Lizard: per artifact sum CCN, NLOC
liz = defaultdict(lambda: [0, 0, 0])
for row in csv.reader((R / "benchmark-40-final-scans/raw/lizard.csv").open(encoding="utf-8", errors="replace")):
    if len(row) > 6 and row[0].isdigit():
        m = re.search(r"(\d{4})\.(py|java|c|cpp)$", row[6])
        if m:
            liz[int(m.group(1))][0] += int(row[1]); liz[int(m.group(1))][1] += int(row[0]); liz[int(m.group(1))][2] += 1
# --- raw Cppcheck: findings per artifact (same exclusions as scan.py)
skip = {"missingInclude", "missingIncludeSystem", "checkersReport", "toomanyconfigs", "normalCheckLevelMaxBranches"}
cpp = defaultdict(int)
for f in ("cppcheck_c.xml", "cppcheck_cpp.xml"):
    for e in ET.parse(R / "benchmark-40-final-scans/raw" / f).getroot().iter("error"):
        loc = e.find("location")
        if loc is not None and e.get("id") not in skip:
            cpp[int(re.search(r"(\d{4})\.\w+$", loc.get("file")).group(1))] += 1

# --- quality by model x language, independently
q = {(r["model"], r["language"]): r for r in csv.DictReader((V2 / "quality_by_model_language.csv").open(encoding="utf-8"))}
for m in models + ["all"]:
    for l in ["python", "java", "c", "cpp", "all"]:
        sel = [c for c, r in inv.items() if g1[c] == "pass" and (m == "all" or r["model_label"] == m) and (l == "all" or r["language"] == l)]
        nloc = sum(liz[c][1] for c in sel)
        fn = [c for c in sel if liz[c][2] > 0]
        cc = [c for c in sel if inv[c]["language"] in ("c", "cpp")]
        nloc_cc = sum(liz[c][1] for c in cc)
        row = q[(m, l)]
        check(f"{m}/{l} gate1_pass_artifacts", int(row["gate1_pass_artifacts"]), len(sel), 0)
        check(f"{m}/{l} nloc_total", int(row["nloc_total"]), nloc, 0)
        check(f"{m}/{l} artifacts_with_functions", int(row["artifacts_with_functions"]), len(fn), 0)
        if fn:
            check(f"{m}/{l} mean_ccd", float(row["mean_ccd"]), round(sum(liz[c][0] / liz[c][1] for c in fn) / len(fn), 4), 1e-4)
            check(f"{m}/{l} pooled_ccd", float(row["pooled_ccd"]), round(sum(liz[c][0] for c in fn) / nloc, 4), 1e-4)
        if nloc_cc:
            check(f"{m}/{l} cppcheck_per_kloc_c_cpp", float(row["cppcheck_per_kloc_c_cpp"]),
                  round(1000 * sum(cpp[c] for c in cc) / nloc_cc, 2), 0.005)
        else:
            check(f"{m}/{l} cppcheck_per_kloc_c_cpp empty", row["cppcheck_per_kloc_c_cpp"], "")

# --- paired gate 1, independently, with SciPy
cells = defaultdict(dict)
for c, r in inv.items():
    cells[(r["task_id"], r["language"])][r["model_label"]] = g1[c] == "pass"
pairs = list(csv.DictReader((V2 / "paired_gate1.csv").open(encoding="utf-8")))
check("pair count", len(pairs), 10, 0)
raw_p = []
for p in pairs:
    a, b = p["model_a"], p["model_b"]
    n10 = sum(1 for v in cells.values() if v[a] and not v[b]); n01 = sum(1 for v in cells.values() if v[b] and not v[a])
    n11 = sum(1 for v in cells.values() if v[a] and v[b]); n00 = len(cells) - n10 - n01 - n11
    for k, want in (("a_only", n10), ("b_only", n01), ("both_pass", n11), ("neither", n00), ("cells", len(cells))):
        check(f"{a}-{b} {k}", int(p[k]), want, 0)
    sp = binomtest(n10, n10 + n01, 0.5, alternative="two-sided").pvalue
    check(f"{a}-{b} p_exact (scipy)", float(p["p_exact_mcnemar"]), sp, max(1e-12, 1e-5 * sp))
    raw_p.append(sp)
    ci = binomtest(n10, n10 + n01, 0.5).proportion_ci(confidence_level=0.95, method="exact")
    lo_or = ci.low / (1 - ci.low)
    hi_or = math.inf if ci.high >= 1 else ci.high / (1 - ci.high)
    check(f"{a}-{b} OR lo (scipy CP)", float(p["or_ci_lo"]), lo_or, 1e-4 * max(1, lo_or))
    if math.isfinite(hi_or):
        check(f"{a}-{b} OR hi (scipy CP)", float(p["or_ci_hi"]), hi_or, 1e-4 * max(1, hi_or))
    d = float(p["diff_a_minus_b"]); lo, hi = float(p["diff_ci_lo"]), float(p["diff_ci_hi"])
    check(f"{a}-{b} diff", d, (n10 - n01) / len(cells), 1e-4)
    if not (lo <= d <= hi and -1 <= lo and hi <= 1):
        problems.append(f"{a}-{b} Newcombe interval [{lo},{hi}] does not contain {d}")
# Holm, independently: compare p_(i) * (m - i + 1) with running maximum
m_ = len(raw_p)
order = sorted(range(m_), key=lambda i: raw_p[i])
adj = [0.0] * m_
prev = 0.0
for rank, i in enumerate(order, start=1):
    prev = max(prev, min(1.0, raw_p[i] * (m_ - rank + 1)))
    adj[i] = prev
for p, want in zip(pairs, adj):
    check(f"{p['model_a']}-{p['model_b']} p_holm", float(p["p_holm"]), want, max(1e-12, 1e-5 * want))
    check(f"{p['model_a']}-{p['model_b']} significant", p["significant_holm_0.05"], str(want < 0.05))

print(f"checked quality cells: {len(q)}, pairs: {len(pairs)}")
print("ALL CHECKS PASS" if not problems else "MISMATCHES:\n  " + "\n  ".join(problems))
sys.exit(1 if problems else 0)
