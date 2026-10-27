#!/usr/bin/env python3
"""Independent check of results/base-40-comparison-v1/cohort_comparison.csv and
paired_cells.csv. Recomputes every value from the raw sources, not from
consolidate.py or compare_cohorts.py:
  gate 1      gate1_results.json (both cohorts)
  security    scans/findings.csv (kind=security, severity high/medium)
  complexity  scans/raw/lizard.csv (unrounded per-function CCN/NLOC)
  cppcheck    scans/raw/cppcheck_*.xml (same excluded ids as scan.py)
  subsets     base_version_classes.csv + published files (escape regex)
Also re-hashes the 600 published originals against the base snapshot.
Exit 1 on any mismatch."""
import csv
import hashlib
import json
import re
import sys
import xml.etree.ElementTree as ET
from collections import defaultdict
from pathlib import Path

from scipy.stats import wilcoxon

EVAL = Path(__file__).resolve().parents[1]
ROOT = EVAL.parent
R = EVAL / "results"
OUT = R / "base-40-comparison-v1"
LANGS = ["c", "cpp", "python"]
SKIP = {"missingInclude", "missingIncludeSystem", "checkersReport", "toomanyconfigs", "normalCheckLevelMaxBranches"}
COMBO = re.compile(r"(?:^|/)(\d{4})\.(py|java|c|cpp)$")
DOUBLE = re.compile(r'\\\\[0nt"\']')
fails = 0


def check(label, got, want, tol=0.0):
    global fails
    if got in ("", None) and want in ("", None):
        return  # both not applicable
    ok = (got == want) if tol == 0 or got in ("", None) or want in ("", None) else abs(float(got) - float(want)) <= tol
    if not ok:
        fails += 1
        print(f"MISMATCH {label}: csv={got!r} recomputed={want!r}")


def cohort(gate1_json, scans):
    g = [r for r in json.loads(Path(gate1_json).read_text(encoding="utf-8")) if r["language"] in LANGS]
    liz = defaultdict(lambda: [0, 0, []])  # nloc, sum_ccn, ccns
    for row in csv.reader(open(scans / "raw/lizard.csv", encoding="utf-8", errors="replace")):
        if len(row) >= 7 and row[0].isdigit():
            m = COMBO.search(row[6].replace("\\", "/"))
            if m:
                p = liz[int(m.group(1))]
                p[0] += int(row[0]); p[1] += int(row[1]); p[2].append(int(row[1]))
    mh = defaultdict(int)
    for f in csv.DictReader(open(scans / "findings.csv", encoding="utf-8")):
        if f["kind"] == "security" and f["severity"] in ("high", "medium"):
            mh[int(f["combo_index"])] += 1
    cpp = defaultdict(int)
    for x in ("cppcheck_c.xml", "cppcheck_cpp.xml"):
        t = (scans / "raw" / x).read_text(encoding="utf-8", errors="replace")
        for e in ET.fromstring(t).iter("error") if t.strip() else []:
            loc = e.find("location")
            if loc is not None and e.get("id") not in SKIP:
                m = COMBO.search(loc.get("file", "").replace("\\", "/"))
                if m:
                    cpp[int(m.group(1))] += 1
    def sym(r):  # re-grade with the ledger-only truncation flag ignored
        if r["gate1_status"] != "fail_integrity" or set(r["integrity_failures"].split(";")) != {"truncated_finish_length"}:
            return r["gate1_status"]
        b, s = r["build_status"], r["syntax_status"]
        return {"toolchain_error": "toolchain_error", "blocked_dependency": "blocked_dependency",
                "error": "fail_syntax" if s == "error" else "fail_compile"}.get(b, "fail_syntax" if s == "error" else "pass")
    return [{"c": r["combo_index"], "task": r["task_id"], "lang": r["language"], "g1": r["gate1_status"], "sym": sym(r),
             "art": r["inventory_status"] in ("generated", "generated_truncated"), "liz": liz.get(r["combo_index"], [0, 0, []]),
             "mh": mh[r["combo_index"]], "cpp": cpp[r["combo_index"]]} for r in g]


def vals(sel, bex, key="g1"):
    sel = [dict(r, g1=r[key]) for r in sel]
    d = [r for r in sel if not (bex and r["g1"] == "blocked_dependency")]
    p = [r for r in d if r["g1"] == "pass"]
    nl = sum(r["liz"][0] for r in p)
    fn = [r for r in p if r["liz"][2]]
    cc = [r for r in p if r["lang"] in ("c", "cpp")]
    ncc = sum(r["liz"][0] for r in cc)
    return {
        "gate1_pass_planned": (len(p), len(d)), "gate1_pass_of_artifacts": (len(p), sum(r["art"] for r in d)),
        "sec_medhigh_of_pass": (sum(r["mh"] > 0 for r in p), len(p)),
        "mean_nloc": nl / len(p) if p else None,
        "mean_avg_ccn": sum(sum(r["liz"][2]) / len(r["liz"][2]) for r in fn) / len(fn) if fn else None,
        "mean_ccd": sum(r["liz"][1] / r["liz"][0] for r in fn) / len(fn) if fn else None,
        "pooled_ccd": sum(r["liz"][1] for r in fn) / nl if nl else None,
        "cppcheck_per_kloc_c_cpp": 1000 * sum(r["cpp"] for r in cc) / ncc if ncc else None,
    }


def main():
    ours = cohort(R / "benchmark-40-final-gate1/gate1_results.json", R / "benchmark-40-final-scans")
    base = cohort(R / "base-40-published-v1/gate1/gate1_results.json", R / "base-40-published-v1/scans")
    inv = {r["combo_index"]: r for r in json.loads((R / "base-40-published-v1/gate1/inventory.json").read_text(encoding="utf-8"))["rows"]}
    cls = {r["published"]: r["class"] for r in csv.DictReader(open(EVAL / "base_compare/base_version_classes.csv", encoding="utf-8"))}
    snap = {a["combo_index"]: a["sha256"] for a in json.loads((R / "base-40-published-v1/SNAPSHOT_base-40-published.json").read_text(encoding="utf-8"))["artifact_list"]}
    changed = 0
    for r in base:
        pub = inv[r["c"]]["published_path"]
        data = (ROOT / pub).read_bytes()
        changed += hashlib.sha256(data).hexdigest() != snap[r["c"]]
        r["ident"] = cls[pub] == "identical"
        r["esc"] = r["lang"] in ("c", "cpp") and bool(DOUBLE.search(data.decode("utf-8", "replace")))
    check("published originals unchanged", changed, 0)
    check("ours rows", len(ours), 600); check("base rows", len(base), 600)
    print(f"base identical={sum(r['ident'] for r in base)} escape={sum(r['esc'] for r in base)}")
    keep = {"primary": (lambda r: True, False), "S1_identical": (lambda r: r["ident"], False),
            "S2_no_escape": (lambda r: not r["esc"], False), "S3_blocked_excl": (lambda r: True, True), "S4_symmetric_integrity": (lambda r: True, False)}
    n = 0
    for row in csv.DictReader(open(OUT / "cohort_comparison.csv", encoding="utf-8")):
        k, bex = keep[row["analysis"]]
        key = "sym" if row["analysis"].startswith("S4") else "g1"
        L = row["language"]
        o = vals([r for r in ours if L == "all" or r["lang"] == L], bex, key)[row["measure"]]
        b = vals([r for r in base if (L == "all" or r["lang"] == L) and k(r)], bex, key)[row["measure"]]
        tag = f"{row['analysis']}/{L}/{row['measure']}"
        if isinstance(o, tuple):
            check(tag + " ours", (int(row["ours_k"]), int(row["ours_n"])), o)
            check(tag + " base", (int(row["base_k"]), int(row["base_n"])), b)
            od = o[0] / o[1] if o[1] else None
            bd = b[0] / b[1] if b[1] else None
            if od is not None and bd is not None:
                check(tag + " diff", row["diff_ours_minus_base"], od - bd, 1e-4)
        else:
            # avg CCN in artifact_scan_summary is rounded to 2 dp per artifact; allow that
            tol = 0.006 if row["measure"] == "mean_avg_ccn" else 1e-4
            check(tag + " ours", row["ours_value"], o, tol)
            check(tag + " base", row["base_value"], b, tol)
        n += 1
    cells = defaultdict(lambda: [0, 0])
    for i, coh in enumerate((ours, base)):
        for r in coh:
            cells[(r["task"], r["lang"])][i] += r["g1"] == "pass"
    for row in csv.DictReader(open(OUT / "paired_cells.csv", encoding="utf-8")):
        check(f"cell {row['task_id']}/{row['language']}", (int(row["ours_models_passing"]), int(row["base_models_passing"])),
              tuple(cells[(int(row["task_id"]), row["language"])]))
    an = json.loads((OUT / "comparison.json").read_text(encoding="utf-8"))["analyses"]
    for name, key in (("primary", "g1"), ("S4_symmetric_integrity", "sym")):
        cl = defaultdict(lambda: [0, 0])
        for i, coh in enumerate((ours, base)):
            for r in coh:
                cl[(r["task"], r["lang"])][i] += r[key] == "pass"
        pc = an[name]["paired_cells"]
        for L in LANGS + ["all"]:
            d = [v[0] - v[1] for k2, v in sorted(cl.items()) if L == "all" or k2[1] == L]
            res = wilcoxon(d, zero_method="wilcox")
            check(f"{name} wilcoxon {L} p", pc[L]["p"], float(res.pvalue), 1e-12)
            check(f"{name} wilcoxon {L} W", pc[L]["wilcoxon_W"], float(res.statistic), 1e-9)
            check(f"{name} wilcoxon {L} counts", (pc[L]["ours_higher"], pc[L]["equal"], pc[L]["base_higher"]),
                  (sum(x > 0 for x in d), sum(x == 0 for x in d), sum(x < 0 for x in d)))
    print(f"{n} comparison rows + {len(cells)} cells + 8 Wilcoxon checked; mismatches: {fails}")
    print("ALL CHECKS PASS" if not fails else "CHECKS FAILED")
    return 1 if fails else 0


if __name__ == "__main__":
    raise SystemExit(main())
