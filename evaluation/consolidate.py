#!/usr/bin/env python3
"""
consolidate.py - Join inventory/cost, gate 1, functional results and static
scans into one row per planned combination, and write model x language
summary tables (CSV + JSON + Markdown).

Evidence is kept in separate columns and never merged into one score:
  confirmed results   gate 1 checks; executed functional tests (passed/failed)
  potential findings  static security scanner hits (unverified)
  failed checks       gate 1 fail_*; functional failed/error/timeout
  blocked checks      gate 1 blocked_dependency
  untested            no artifact, or not_testable with its reason

Usage:
  python consolidate.py --gate1 results/benchmark-40-final-gate1 \
     --functional results/benchmark-40-final-functional --scans results/benchmark-40-final-scans \
     --snapshot results/SNAPSHOT_benchmark-40-final.json --out results/benchmark-40-final-consolidated
"""

import argparse
import csv
import io
import json
import math
import re
from collections import Counter, defaultdict
from pathlib import Path

from evaluate import wilson, mcnemar_exact

# Version 1 produced results/benchmark-40-final-consolidated (its cppcheck_per_kloc
# divided C/C++ findings by the NLOC of all languages). Version 2 fixes that and adds
# complexity density, per-language quality tables and paired gate-1 statistics.
CONSOLIDATE_VERSION = 2
LANGS = ["python", "java", "c", "cpp"]
C_FAMILY = ("c", "cpp")


# ── statistics for paired binary outcomes ────────────────────────────────────

def binom_tail_two_sided(b: int, c: int) -> float:
    """Exact two-sided McNemar p-value: binomial test of b successes in b+c
    discordant pairs with p = 0.5 (full precision, no rounding)."""
    n = b + c
    if n == 0:
        return 1.0
    return min(1.0, 2 * sum(math.comb(n, i) for i in range(0, min(b, c) + 1)) / 2 ** n)


def holm(pvalues: list[float]) -> list[float]:
    """Holm step-down adjusted p-values (monotone, capped at 1)."""
    m = len(pvalues)
    order = sorted(range(m), key=lambda i: pvalues[i])
    adjusted, running = [0.0] * m, 0.0
    for rank, i in enumerate(order):
        running = max(running, min(1.0, (m - rank) * pvalues[i]))
        adjusted[i] = running
    return adjusted


def _wilson_raw(k: int, n: int, z: float = 1.959963984540054) -> tuple[float, float]:
    p = k / n
    denom = 1 + z * z / n
    centre = (p + z * z / (2 * n)) / denom
    half = z * math.sqrt(p * (1 - p) / n + z * z / (4 * n * n)) / denom
    return max(0.0, centre - half), min(1.0, centre + half)


def newcombe_paired_diff(a: int, b: int, c: int, d: int) -> tuple[float, float, float]:
    """Difference of paired proportions p1 - p2 with Newcombe's (1998) method-10
    hybrid score interval. a = both pass, b = only first passes, c = only second
    passes, d = neither."""
    n = a + b + c + d
    p1, p2 = (a + b) / n, (a + c) / n
    l1, u1 = _wilson_raw(a + b, n)
    l2, u2 = _wilson_raw(a + c, n)
    den = (a + b) * (c + d) * (a + c) * (b + d)
    phi = (a * d - b * c) / math.sqrt(den) if den else 0.0
    diff = p1 - p2
    lo = diff - math.sqrt(max(0.0, (p1 - l1) ** 2 - 2 * phi * (p1 - l1) * (u2 - p2) + (u2 - p2) ** 2))
    hi = diff + math.sqrt(max(0.0, (u1 - p1) ** 2 - 2 * phi * (u1 - p1) * (p2 - l2) + (p2 - l2) ** 2))
    return diff, max(-1.0, lo), min(1.0, hi)


def _binom_cdf(k: int, n: int, p: float) -> float:
    return sum(math.comb(n, i) * p ** i * (1 - p) ** (n - i) for i in range(0, k + 1))


def clopper_pearson(k: int, n: int, alpha: float = 0.05) -> tuple[float, float]:
    """Exact binomial interval by bisection on the binomial CDF."""
    def solve(f):
        lo, hi = 0.0, 1.0
        for _ in range(200):
            mid = (lo + hi) / 2
            lo, hi = (mid, hi) if f(mid) else (lo, mid)
        return (lo + hi) / 2
    lower = 0.0 if k == 0 else solve(lambda p: 1 - _binom_cdf(k - 1, n, p) < alpha / 2)
    upper = 1.0 if k == n else solve(lambda p: _binom_cdf(k, n, p) > alpha / 2)
    return lower, upper


def conditional_odds_ratio(b: int, c: int) -> tuple[float | None, float | None, float | None]:
    """McNemar conditional odds ratio b/c with an exact interval from the
    Clopper-Pearson interval of b/(b+c) (OR = q/(1-q)). None when undefined."""
    if b + c == 0:
        return None, None, None
    lo_q, hi_q = clopper_pearson(b, b + c)
    to_or = lambda q: math.inf if q >= 1.0 else q / (1 - q)
    return (math.inf if c == 0 else b / c), to_or(lo_q), to_or(hi_q)


def lizard_by_artifact(lizard_csv: Path) -> dict:
    """{combo_index: (sum_ccn, function_nloc, n_functions)} from the raw Lizard CSV."""
    per = defaultdict(lambda: [0, 0, 0])
    for row in csv.reader(io.StringIO(lizard_csv.read_text(encoding="utf-8", errors="replace"))):
        if len(row) < 7 or not row[0].isdigit():
            continue
        m = re.search(r"(?:^|/)(\d{4})\.(py|java|c|cpp)$", row[6].replace("\\", "/"))
        if m:
            p = per[int(m.group(1))]
            p[0] += int(row[1]); p[1] += int(row[0]); p[2] += 1
    return {k: tuple(v) for k, v in per.items()}
ROW_FIELDS = [
    "combo_index", "task_id", "language", "model_label", "model_id",
    "generation_status", "attempts_total", "billed_attempts", "cost_usd_total", "output_tokens", "truncated",
    "gate1_status", "gate1_first_error", "missing_dependencies",
    "oracle_status", "oracle_test", "adapter_status", "functional_status", "functional_reason",
    "nloc", "sum_ccn", "ccd", "avg_ccn", "max_ccn", "functions_ccn_gt10", "cppcheck_findings", "cppcheck_errors",
    "sec_findings", "sec_high", "sec_medium", "sec_low", "sec_per_kloc", "semgrep", "bandit", "flawfinder", "cwes",
    "evidence_generation", "evidence_gate1", "evidence_functional", "evidence_security",
]


METRICS_MD = """# Metric definitions (consolidate.py version 2)

All quality metrics are computed on gate-1-passing artifacts only.

| Metric | Definition | Source |
|---|---|---|
| NLOC | Sum of Lizard's per-function NLOC (non-blank, non-comment lines inside functions). Top-level code outside functions is **not** counted. | `raw/lizard.csv` |
| mean NLOC | NLOC summed over the group's gate-1-passing artifacts, divided by their number (artifacts without functions count as 0). | |
| avg CCN (per artifact) | Mean Lizard cyclomatic complexity over the artifact's functions. | `artifact_scan_summary.csv` |
| mean avg CCN | Mean of per-artifact avg CCN over artifacts with ≥1 function. | |
| sum CCN (per artifact) | Sum of Lizard CCN over the artifact's functions. | `raw/lizard.csv` |
| CCD (per artifact) | Cyclomatic complexity density = sum CCN / NLOC, following the base paper's Md = M / cl, with cl = Lizard function NLOC (not file LOC). Undefined for artifacts without functions. | |
| mean CCD | Mean of per-artifact CCD over artifacts with ≥1 function. | |
| pooled CCD | Sum of CCN / sum of NLOC over the group. | |
| functions CCN > 10 | Count of functions with Lizard CCN > 10. | |
| Cppcheck findings / KLOC (C/C++) | 1000 × Cppcheck findings / NLOC of the group's **C and C++** artifacts only. Not applicable to Python/Java. Version 1 divided by all-language NLOC (understated ~2.2×). | `raw/cppcheck_*.xml` |

## Unavailable metrics (not measured; no tool ran that produces them)

- Cognitive complexity (base paper via SonarQube).
- Maintainability, reliability (bugs), code smells and security hotspots (SonarQube).
- Any linter or quality findings for Python and Java (Cppcheck covers C/C++ only).
- File-level LOC including top-level code (Lizard CSV output is per function).
- Code coverage (base paper used JaCoCo on a subset).
- Maintainability index, Halstead metrics.
- Token count and parameter count are in `raw/lizard.csv` but are not aggregated.

## Paired gate-1 statistics

Unit: (task, language) cell, 160 cells, each model's outcome = gate 1 pass (a failed generation counts as not passing).
p: exact McNemar test (two-sided binomial test on discordant cells with p = 0.5).
Holm step-down adjustment over the 10 model pairs.
Effect sizes: paired difference in pass rate with Newcombe (1998) method-10 interval, and conditional odds ratio a_only/b_only with an exact interval derived from the Clopper–Pearson interval of a_only/(a_only+b_only).
Caveat: the four language cells of one task are not independent; the tests treat the 160 cells as independent pairs.
"""


def rate(k, n):
    lo, hi = wilson(k, n)
    return {"k": k, "n": n, "rate": round(k / n, 4) if n else None, "ci95": [lo, hi]}


def fmt(r):
    if not r["n"]:
        return "n/a"
    lo, hi = r["ci95"]
    return f"{r['k']}/{r['n']} = {100 * r['rate']:.1f}% [{100 * lo:.1f}–{100 * hi:.1f}]"


def load_csv(path):
    with open(path, encoding="utf-8", newline="") as fh:
        return list(csv.DictReader(fh))


def evidence(row):
    g = row["gate1_status"]
    if row["generation_status"] not in ("generated", "generated_truncated"):
        gen = "failed_generation"
    else:
        gen = "truncated_output" if row["generation_status"] == "generated_truncated" else "generated"
    gate = ("untested:no_artifact" if g == "no_artifact" else "confirmed_pass" if g == "pass" else
            "blocked_check" if g == "blocked_dependency" else f"failed_check:{g}")
    f = row["functional_status"]
    func = ("confirmed_pass" if f == "passed" else "confirmed_fail" if f == "failed" else
            f"failed_check:{f}" if f in ("error", "timeout") else f"untested:{row['functional_reason'].split(':')[0].split(' (')[0]}")
    if row["nloc"] == "" and row["sec_findings"] == "":
        sec = "untested:no_artifact"
    elif int(row["sec_findings"] or 0):
        sec = f"potential_findings:{row['sec_findings']} (high {row['sec_high']})"
    else:
        sec = "no_scanner_findings (not proof of security)"
    return gen, gate, func, sec


def main(argv=None) -> int:
    ap = argparse.ArgumentParser()
    for a in ("--gate1", "--functional", "--scans", "--snapshot", "--out"):
        ap.add_argument(a, required=True)
    args = ap.parse_args(argv)
    out = Path(args.out)
    if out.exists() and any(out.iterdir()):
        raise SystemExit(f"Output folder already has files: {out}")
    out.mkdir(parents=True, exist_ok=True)
    inv = json.loads((Path(args.gate1) / "inventory.json").read_text(encoding="utf-8"))["rows"]
    gate1 = {r["combo_index"]: r for r in json.loads((Path(args.gate1) / "gate1_results.json").read_text(encoding="utf-8"))}
    func = {int(r["combo_index"]): r for r in load_csv(Path(args.functional) / "functional_plan.csv")}
    scans = {int(r["combo_index"]): r for r in load_csv(Path(args.scans) / "artifact_scan_summary.csv")}
    lizard = lizard_by_artifact(Path(args.scans) / "raw" / "lizard.csv")
    snap = json.loads(Path(args.snapshot).read_text(encoding="utf-8"))
    models = sorted({r["model_label"] for r in inv})

    rows = []
    for r in inv:
        c = r["combo_index"]
        g, f, s = gate1[c], func.get(c, {}), scans.get(c, {})
        row = {k: "" for k in ROW_FIELDS}
        row.update({k: r[k] for k in ("combo_index", "task_id", "language", "model_label", "model_id",
                                      "attempts_total", "billed_attempts", "cost_usd_total", "output_tokens", "truncated")})
        row.update({"generation_status": r["status"], "gate1_status": g["gate1_status"],
                    "gate1_first_error": g.get("first_error", ""), "missing_dependencies": g.get("missing_dependencies", ""),
                    "oracle_status": f.get("oracle_status", ""), "oracle_test": f.get("oracle_test_path", ""),
                    "adapter_status": f.get("adapter_status", ""), "functional_status": f.get("functional_status", "not_testable"),
                    "functional_reason": f.get("reason", "")})
        for k in ("nloc", "avg_ccn", "max_ccn", "functions_ccn_gt10", "cppcheck_findings", "cppcheck_errors", "sec_findings",
                  "sec_high", "sec_medium", "sec_low", "sec_per_kloc", "semgrep", "bandit", "flawfinder", "cwes"):
            row[k] = s.get(k, "")
        if c in lizard:
            sum_ccn, fn_nloc, _ = lizard[c]
            if str(fn_nloc) != str(row["nloc"]):
                raise SystemExit(f"Lizard NLOC mismatch for combo {c}: raw {fn_nloc} vs summary {row['nloc']}")
            row["sum_ccn"] = sum_ccn
            row["ccd"] = round(sum_ccn / fn_nloc, 4) if fn_nloc else ""
        row["evidence_generation"], row["evidence_gate1"], row["evidence_functional"], row["evidence_security"] = evidence(row)
        rows.append(row)
    with (out / "combined_per_combination.csv").open("w", encoding="utf-8", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=ROW_FIELDS, lineterminator="\n")
        w.writeheader()
        w.writerows(rows)
    (out / "combined_per_combination.json").write_text(json.dumps(rows, indent=1) + "\n", encoding="utf-8")

    def stats(sel):
        n = len(sel)
        arts = [r for r in sel if r["generation_status"] in ("generated", "generated_truncated")]
        g1 = [r for r in sel if r["gate1_status"] == "pass"]
        executed = [r for r in sel if r["functional_status"] in ("passed", "failed", "error", "timeout")]
        fpass = [r for r in executed if r["functional_status"] == "passed"]
        scanned = g1  # every gate-1-passing artifact was scanned (scan_manifest: 734/734); NLOC only for per-KLOC rates
        sec_any = [r for r in scanned if int(r["sec_findings"] or 0) > 0]
        sec_high = [r for r in scanned if int(r["sec_high"] or 0) > 0]
        sec_mh = [r for r in scanned if int(r["sec_high"] or 0) + int(r["sec_medium"] or 0) > 0]
        mh_count = sum(int(r["sec_high"] or 0) + int(r["sec_medium"] or 0) for r in scanned)
        nloc = sum(int(r["nloc"] or 0) for r in scanned)
        cost = sum(float(r["cost_usd_total"] or 0) for r in sel)
        ccn = [float(r["avg_ccn"]) for r in scanned if r["avg_ccn"] not in ("", None)]
        nloc_cc = sum(int(r["nloc"] or 0) for r in scanned if r["language"] in C_FAMILY)
        with_fn = [r for r in scanned if r["ccd"] not in ("", None)]
        return {
            "planned": n, "artifacts": len(arts),
            "truncated": sum(1 for r in sel if r["generation_status"] == "generated_truncated"),
            "failed_generation": n - len(arts),
            "gate1_counts": dict(Counter(r["gate1_status"] for r in sel)),
            "gate1_pass": rate(len(g1), n),
            "functional_counts": dict(Counter(r["functional_status"] for r in sel)),
            "functional_executed": len(executed),
            "functional_pass_of_executed": rate(len(fpass), len(executed)),
            "untested_reasons": dict(Counter(r["evidence_functional"].split(":", 1)[1] for r in sel
                                             if r["evidence_functional"].startswith("untested"))),
            "scanned_gate1_pass": len(scanned),
            "security_any_finding": rate(len(sec_any), len(scanned)),
            "security_high_finding": rate(len(sec_high), len(scanned)),
            "security_medhigh_finding": rate(len(sec_mh), len(scanned)),
            "security_medhigh_per_kloc": round(1000 * mh_count / nloc, 2) if nloc else None,
            "security_findings_per_kloc": round(1000 * sum(int(r["sec_findings"] or 0) for r in scanned) / nloc, 2) if nloc else None,
            "findings_by_tool": {t: sum(int(r[t] or 0) for r in scanned) for t in ("semgrep", "bandit", "flawfinder")},
            # Cppcheck analyses C/C++ only, so its density uses C/C++ function NLOC only (fixed in version 2).
            "cppcheck_findings_c_cpp": sum(int(r["cppcheck_findings"] or 0) for r in scanned if r["language"] in C_FAMILY),
            "nloc_c_cpp": nloc_cc,
            "cppcheck_per_kloc_c_cpp": (round(1000 * sum(int(r["cppcheck_findings"] or 0) for r in scanned
                                                         if r["language"] in C_FAMILY) / nloc_cc, 2) if nloc_cc else None),
            "nloc_total": nloc,
            "mean_nloc": round(nloc / len(scanned), 1) if scanned else None,
            "artifacts_with_functions": len(with_fn),
            "mean_avg_ccn": round(sum(ccn) / len(ccn), 2) if ccn else None,
            "mean_ccd": round(sum(float(r["ccd"]) for r in with_fn) / len(with_fn), 4) if with_fn else None,
            "pooled_ccd": round(sum(int(r["sum_ccn"]) for r in with_fn) / nloc, 4) if nloc else None,
            "functions_ccn_gt10": sum(int(r["functions_ccn_gt10"] or 0) for r in scanned),
            "cost_usd": round(cost, 6),
            "cost_per_gate1_pass": round(cost / len(g1), 6) if g1 else None,
            "cost_per_functional_pass": round(cost / len(fpass), 6) if fpass else None,
            "output_tokens": sum(int(r["output_tokens"] or 0) for r in sel),
        }

    summary = {
        "snapshot_id": snap["snapshot_id"],
        "overall": stats(rows),
        "by_model": {m: stats([r for r in rows if r["model_label"] == m]) for m in models},
        "by_language": {l: stats([r for r in rows if r["language"] == l]) for l in LANGS},
        "by_model_language": {m: {l: stats([r for r in rows if r["model_label"] == m and r["language"] == l])
                                  for l in LANGS} for m in models},
    }
    # Paired functional comparison: (task, language) cells where both models' tests were executed.
    cells = defaultdict(dict)
    for r in rows:
        if r["functional_status"] in ("passed", "failed", "error", "timeout"):
            cells[(r["task_id"], r["language"])][r["model_label"]] = r["functional_status"] == "passed"
    pairs = []
    for i, a in enumerate(models):
        for b in models[i + 1:]:
            both = [c for c in cells.values() if a in c and b in c]
            ab = sum(1 for c in both if c[a] and not c[b]); ba = sum(1 for c in both if c[b] and not c[a])
            pairs.append({"model_a": a, "model_b": b, "paired_cells": len(both), "a_pass_b_not": ab, "b_pass_a_not": ba,
                          "mcnemar_exact_p_unadjusted": mcnemar_exact(ab, ba)})
    summary["paired_functional"] = pairs
    # Paired gate-1 comparison (version 2). Unit: (task, language) cell; outcome:
    # gate 1 pass (a failed generation counts as not passing). Exact McNemar test,
    # Holm adjustment over all model pairs, Newcombe interval for the paired
    # difference, exact interval for the conditional odds ratio.
    g1cells = defaultdict(dict)
    for r in rows:
        g1cells[(r["task_id"], r["language"])][r["model_label"]] = r["gate1_status"] == "pass"
    complete = [c for c in g1cells.values() if all(m in c for m in models)]
    g1pairs = []
    for i, a in enumerate(models):
        for b in models[i + 1:]:
            n11 = sum(1 for c in complete if c[a] and c[b]); n10 = sum(1 for c in complete if c[a] and not c[b])
            n01 = sum(1 for c in complete if not c[a] and c[b]); n00 = sum(1 for c in complete if not c[a] and not c[b])
            diff, d_lo, d_hi = newcombe_paired_diff(n11, n10, n01, n00)
            or_, or_lo, or_hi = conditional_odds_ratio(n10, n01)
            g1pairs.append({"model_a": a, "model_b": b, "cells": len(complete), "both_pass": n11, "a_only": n10,
                            "b_only": n01, "neither": n00, "pass_rate_a": (n11 + n10) / len(complete),
                            "pass_rate_b": (n11 + n01) / len(complete), "diff_a_minus_b": diff,
                            "diff_ci95": [d_lo, d_hi], "conditional_or": or_, "conditional_or_ci95": [or_lo, or_hi],
                            "p_exact_mcnemar": binom_tail_two_sided(n10, n01)})
    for p, adj in zip(g1pairs, holm([p["p_exact_mcnemar"] for p in g1pairs])):
        p["p_holm"] = adj
        p["significant_holm_0.05"] = adj < 0.05
    summary["paired_gate1"] = {
        "method": "exact McNemar (two-sided binomial on discordant cells, p=0.5); Holm step-down over all "
                  f"{len(g1pairs)} model pairs; paired difference with Newcombe (1998) method-10 interval; "
                  "conditional odds ratio a_only/b_only with exact Clopper-Pearson-derived interval",
        "unit": "(task, language) cell; outcome = gate 1 pass; failed generation = not pass",
        "caveat": "cells from the same task are not independent across languages; tests treat the 160 cells as independent pairs",
        "complete_cells": len(complete), "pairs": g1pairs}
    with (out / "paired_gate1.csv").open("w", encoding="utf-8", newline="") as fh:
        w = csv.writer(fh, lineterminator="\n")
        w.writerow(["model_a", "model_b", "cells", "both_pass", "a_only", "b_only", "neither", "pass_rate_a", "pass_rate_b",
                    "diff_a_minus_b", "diff_ci_lo", "diff_ci_hi", "conditional_or", "or_ci_lo", "or_ci_hi",
                    "p_exact_mcnemar", "p_holm", "significant_holm_0.05"])
        for p in g1pairs:
            f6 = lambda x: "inf" if x == math.inf else ("" if x is None else f"{x:.6g}")
            w.writerow([p["model_a"], p["model_b"], p["cells"], p["both_pass"], p["a_only"], p["b_only"], p["neither"],
                        f"{p['pass_rate_a']:.4f}", f"{p['pass_rate_b']:.4f}", f"{p['diff_a_minus_b']:.4f}",
                        f"{p['diff_ci95'][0]:.4f}", f"{p['diff_ci95'][1]:.4f}", f6(p["conditional_or"]),
                        f6(p["conditional_or_ci95"][0]), f6(p["conditional_or_ci95"][1]),
                        f"{p['p_exact_mcnemar']:.6g}", f"{p['p_holm']:.6g}", p["significant_holm_0.05"]])
    for p in g1pairs:  # JSON has no infinity: an odds ratio with no b_only cells is written as "inf"
        p["conditional_or"] = "inf" if p["conditional_or"] == math.inf else p["conditional_or"]
        p["conditional_or_ci95"] = ["inf" if x == math.inf else x for x in p["conditional_or_ci95"]]
    summary["consolidate_version"] = CONSOLIDATE_VERSION
    # Top CWEs (potential) among gate-1-passing artifacts
    cwe = Counter()
    for r in rows:
        if r["gate1_status"] == "pass":
            for c in (r["cwes"] or "").split(";"):
                if c:
                    cwe[c] += 1
    summary["top_cwes_artifacts"] = cwe.most_common(15)
    (out / "summary.json").write_text(json.dumps(summary, indent=1) + "\n", encoding="utf-8")

    # Flat tables
    with (out / "table_by_model.csv").open("w", encoding="utf-8", newline="") as fh:
        w = csv.writer(fh, lineterminator="\n")
        w.writerow(["model", "planned", "artifacts", "truncated", "failed_generation", "gate1_pass", "gate1_rate", "gate1_ci_lo",
                    "gate1_ci_hi", "functional_executed", "functional_pass", "functional_rate", "func_ci_lo", "func_ci_hi",
                    "scanned", "sec_medhigh", "sec_medhigh_rate", "sec_medhigh_ci_lo", "sec_medhigh_ci_hi", "sec_high", "sec_high_rate",
                    "sec_any_incl_low", "sec_any_rate", "sec_medhigh_per_kloc", "sec_all_per_kloc", "cppcheck_per_kloc_c_cpp",
                    "mean_nloc", "mean_avg_ccn", "mean_ccd", "pooled_ccd", "cost_usd", "cost_per_gate1_pass",
                    "cost_per_functional_pass", "output_tokens"])
        for m, s in summary["by_model"].items():
            g, fp, sa, sh = s["gate1_pass"], s["functional_pass_of_executed"], s["security_any_finding"], s["security_high_finding"]
            mh = s["security_medhigh_finding"]
            w.writerow([m, s["planned"], s["artifacts"], s["truncated"], s["failed_generation"], g["k"], g["rate"], *g["ci95"],
                        s["functional_executed"], fp["k"], fp["rate"], *fp["ci95"], s["scanned_gate1_pass"], mh["k"], mh["rate"],
                        *mh["ci95"], sh["k"], sh["rate"], sa["k"], sa["rate"], s["security_medhigh_per_kloc"],
                        s["security_findings_per_kloc"], s["cppcheck_per_kloc_c_cpp"], s["mean_nloc"],
                        s["mean_avg_ccn"], s["mean_ccd"], s["pooled_ccd"], s["cost_usd"], s["cost_per_gate1_pass"],
                        s["cost_per_functional_pass"], s["output_tokens"]])
    with (out / "table_by_model_language.csv").open("w", encoding="utf-8", newline="") as fh:
        w = csv.writer(fh, lineterminator="\n")
        w.writerow(["model", "language", "planned", "artifacts", "gate1_pass", "gate1_rate", "functional_executed",
                    "functional_pass", "functional_rate", "scanned", "sec_medhigh", "sec_medhigh_rate", "sec_high", "sec_any_incl_low",
                    "sec_medhigh_per_kloc", "cppcheck_per_kloc_c_cpp", "mean_nloc", "functions_ccn_gt10",
                    "artifacts_with_functions", "mean_avg_ccn", "mean_ccd", "pooled_ccd", "cost_usd"])
        for m, langs in summary["by_model_language"].items():
            for l, s in langs.items():
                w.writerow([m, l, s["planned"], s["artifacts"], s["gate1_pass"]["k"], s["gate1_pass"]["rate"],
                            s["functional_executed"], s["functional_pass_of_executed"]["k"], s["functional_pass_of_executed"]["rate"],
                            s["scanned_gate1_pass"], s["security_medhigh_finding"]["k"], s["security_medhigh_finding"]["rate"],
                            s["security_high_finding"]["k"], s["security_any_finding"]["k"], s["security_medhigh_per_kloc"],
                            "" if s["cppcheck_per_kloc_c_cpp"] is None else s["cppcheck_per_kloc_c_cpp"],
                            s["mean_nloc"], s["functions_ccn_gt10"], s["artifacts_with_functions"], s["mean_avg_ccn"],
                            s["mean_ccd"], s["pooled_ccd"], s["cost_usd"]])
    # Quality per model x language (version 2), including language totals.
    with (out / "quality_by_model_language.csv").open("w", encoding="utf-8", newline="") as fh:
        w = csv.writer(fh, lineterminator="\n")
        w.writerow(["model", "language", "gate1_pass_artifacts", "artifacts_with_functions", "nloc_total", "mean_nloc",
                    "mean_avg_ccn", "mean_ccd", "pooled_ccd", "functions_ccn_gt10", "cppcheck_findings_c_cpp",
                    "cppcheck_per_kloc_c_cpp"])
        groups = [(m, l, summary["by_model_language"][m][l]) for m in models for l in LANGS]
        groups += [("all", l, summary["by_language"][l]) for l in LANGS]
        groups += [(m, "all", summary["by_model"][m]) for m in models] + [("all", "all", summary["overall"])]
        for m, l, s in groups:
            na = lambda v: "" if v is None else v
            w.writerow([m, l, s["scanned_gate1_pass"], s["artifacts_with_functions"],
                        s["nloc_total"], na(s["mean_nloc"]), na(s["mean_avg_ccn"]),
                        na(s["mean_ccd"]), na(s["pooled_ccd"]), s["functions_ccn_gt10"],
                        s["cppcheck_findings_c_cpp"] if l in C_FAMILY + ("all",) else "",
                        na(s["cppcheck_per_kloc_c_cpp"])])
    (out / "METRICS.md").write_text(METRICS_MD, encoding="utf-8")

    # Markdown report
    md = [f"# Benchmark-40 evaluation results", "", f"Snapshot `{snap['snapshot_id'][:16]}…`: 800 planned combinations "
          "(40 tasks × 5 models × 4 languages). Rates carry Wilson 95% intervals.", "",
          "**How to read this.** Gate 1 and executed functional tests are confirmed results. Static security "
          "findings are *potential* issues reported by scanners; they are not verified vulnerabilities, and an artifact "
          "with no findings is not shown to be secure. Untested and blocked combinations are counted separately and are "
          "not treated as failures or passes.", "",
          "## 1. Generation, gate 1 and cost by model", "",
          "| model | artifacts / 160 | truncated | gate 1 pass | cost (USD) | cost per gate-1 pass |", "|---|---|---|---|---|---|"]
    for m, s in summary["by_model"].items():
        md.append(f"| {m} | {s['artifacts']} | {s['truncated']} | {fmt(s['gate1_pass'])} | {s['cost_usd']:.4f} | "
                  f"{s['cost_per_gate1_pass'] if s['cost_per_gate1_pass'] is not None else 'n/a'} |")
    md += ["", "## 2. Gate 1 pass by model and language", "", "| model | " + " | ".join(LANGS) + " |", "|---|" + "---|" * 4]
    for m, langs in summary["by_model_language"].items():
        md.append(f"| {m} | " + " | ".join(fmt(langs[l]["gate1_pass"]) for l in LANGS) + " |")
    md += ["", "## 3. Functional tests (executed in the verified sandbox, approved oracles and adapters only)", "",
           "| model | executed | passed | failed | error | timeout | pass rate of executed |", "|---|---|---|---|---|---|---|"]
    for m, s in summary["by_model"].items():
        fc = s["functional_counts"]
        md.append(f"| {m} | {s['functional_executed']} | {fc.get('passed', 0)} | {fc.get('failed', 0)} | {fc.get('error', 0)} | "
                  f"{fc.get('timeout', 0)} | {fmt(s['functional_pass_of_executed'])} |")
    md += ["", "Functional results by language (pass / executed):", "", "| model | " + " | ".join(LANGS) + " |", "|---|" + "---|" * 4]
    for m, langs in summary["by_model_language"].items():
        md.append(f"| {m} | " + " | ".join(f"{langs[l]['functional_pass_of_executed']['k']}/{langs[l]['functional_executed']}"
                                             for l in LANGS) + " |")
    o = summary["overall"]
    md += ["", "Why combinations were not functionally tested (all models):", "",
           *[f"- {k}: {v}" for k, v in sorted(o["untested_reasons"].items(), key=lambda x: -x[1])], "",
           "## 4. Potential security findings (static scanners, gate-1-passing artifacts)", "",
           "Headline metric: share of artifacts with at least one medium- or high-severity potential finding. Low-severity "
           "hits (e.g. flawfinder level 0-1 printf with constant formats, bandit assert_used) are kept in findings.csv but not headlined.", "",
           "| model | scanned | ≥1 medium/high | ≥1 high | ≥1 any (incl. low) | medium+high / KLOC | semgrep | bandit | flawfinder |",
           "|---|---|---|---|---|---|---|---|---|"]
    for m, s in summary["by_model"].items():
        t = s["findings_by_tool"]
        md.append(f"| {m} | {s['scanned_gate1_pass']} | {fmt(s['security_medhigh_finding'])} | {fmt(s['security_high_finding'])} | "
                  f"{fmt(s['security_any_finding'])} | {s['security_medhigh_per_kloc']} | {t['semgrep']} | {t['bandit']} | {t['flawfinder']} |")
    md += ["", "By language (≥1 medium/high finding among scanned gate-1-passing artifacts):", "", "| model | " + " | ".join(LANGS) + " |",
           "|---|" + "---|" * 4]
    for m, langs in summary["by_model_language"].items():
        md.append(f"| {m} | " + " | ".join(fmt(langs[l]["security_medhigh_finding"]) for l in LANGS) + " |")
    md += ["", "Most frequent CWEs (number of gate-1-passing artifacts with at least one finding):", "",
           ", ".join(f"{c} ({n})" for c, n in summary["top_cwes_artifacts"]), "",
           "flawfinder flags risky C APIs by name (e.g. strcpy, memcpy) and dominates C/C++ counts; compare tools separately.", "",
           "## 5. Quality (gate-1-passing artifacts)", "",
           "Definitions and unavailable metrics: METRICS.md. Per-language values: quality_by_model_language.csv.", "",
           "| model | mean NLOC | mean per-file avg CCN | mean CCD | functions with CCN > 10 | Cppcheck findings / KLOC (C/C++ NLOC only) |",
           "|---|---|---|---|---|---|"]
    for m, s in summary["by_model"].items():
        md.append(f"| {m} | {s['mean_nloc']} | {s['mean_avg_ccn']} | {s['mean_ccd']} | {s['functions_ccn_gt10']} | "
                  f"{s['cppcheck_per_kloc_c_cpp']} |")
    md += ["", "## 5b. Paired gate-1 comparison (exact McNemar, Holm-adjusted)", "",
           "| A | B | A only | B only | diff A−B [95% CI] | conditional OR [95% CI] | p | p (Holm) |",
           "|---|---|---|---|---|---|---|---|"]
    for p in summary["paired_gate1"]["pairs"]:
        orv = p["conditional_or"]; ol, oh = p["conditional_or_ci95"]
        fo = lambda x: x if isinstance(x, str) else f"{x:.2f}"
        md.append(f"| {p['model_a']} | {p['model_b']} | {p['a_only']} | {p['b_only']} | "
                  f"{100 * p['diff_a_minus_b']:+.1f} pp [{100 * p['diff_ci95'][0]:+.1f}, {100 * p['diff_ci95'][1]:+.1f}] | "
                  f"{fo(orv)} [{fo(ol)}, {fo(oh)}] | {p['p_exact_mcnemar']:.3g} | {p['p_holm']:.3g} |")
    md += ["", "## 6. Cost versus quality", "",
           "| model | cost (USD) | output tokens | gate 1 pass rate | functional pass rate (executed) | ≥1 medium/high security finding | cost per functional pass |",
           "|---|---|---|---|---|---|---|"]
    for m, s in summary["by_model"].items():
        md.append(f"| {m} | {s['cost_usd']:.4f} | {s['output_tokens']} | {100 * (s['gate1_pass']['rate'] or 0):.1f}% | "
                  f"{(str(round(100 * s['functional_pass_of_executed']['rate'], 1)) + '%') if s['functional_pass_of_executed']['n'] else 'n/a'} | "
                  f"{(str(round(100 * s['security_medhigh_finding']['rate'], 1)) + '%') if s['security_medhigh_finding']['n'] else 'n/a'} | "
                  f"{s['cost_per_functional_pass'] if s['cost_per_functional_pass'] is not None else 'n/a'} |")
    md += ["", "Costs are the ledger-recorded generation cost per model, including failed and retried attempts. They exclude "
           "$0.0655 provisional and $0.13 unreconciled spend that cannot be attributed to a model.", "",
           "## 7. Paired functional comparison (exploratory)", "",
           "Exact McNemar on (task, language) cells where both models' tests were executed; unadjusted for multiple comparisons.", "",
           "| A | B | paired cells | A pass, B not | B pass, A not | p |", "|---|---|---|---|---|---|",
           *[f"| {p['model_a']} | {p['model_b']} | {p['paired_cells']} | {p['a_pass_b_not']} | {p['b_pass_a_not']} | "
             f"{p['mcnemar_exact_p_unadjusted']} |" for p in pairs], "",
           "## 8. Evidence classes (all 800 combinations)", ""]
    for col in ("evidence_gate1", "evidence_functional", "evidence_security"):
        c = Counter(r[col].split(" (")[0].split(":")[0] for r in rows)
        md.append(f"- **{col.replace('evidence_', '')}**: " + ", ".join(f"{k} {v}" for k, v in c.most_common()))
    md += ["", "## Limits", "",
           "- Oracle and adapter decisions were made by an AI-assisted review on the user's instruction and need human confirmation before publication (`functional/oracle_review.json`, `functional/adapter_review.json`).",
           "- Functional coverage is partial: only tasks with an approved, recovered upstream test and an approved name-only adapter were executed. Pass rates are conditional on being testable and are not comparable to rates over all 800 combinations.",
           "- Gate 1 compiles C/C++ with clang 20 for Linux without linking; functional builds use gcc 13 in the sandbox.",
           "- Static scanners: semgrep (pinned cwe-top-25 + security-audit), bandit, flawfinder, cppcheck. CodeQL and SonarQube were not run. Scanner findings were not manually triaged.",
           "- Truncated outputs (finish_reason=length) are gate-1 failures; scans of them are excluded from the security/quality tables.",
           "- Functional tests show a ceiling effect: every executed test passed for every model, so the recovered tests (mostly well-known kata tasks) do not discriminate between models. Security-relevant tasks mostly had no usable, spec-consistent oracle.",
           "- NLOC is lizard's sum over functions; top-level script code is not counted (8 gate-1-passing artifacts have NLOC 0), so per-KLOC rates are approximate.",
           "- flawfinder reports risky C APIs and fixed-size buffers by name (CWE-119/120/134); it drives the high C rates and is a known high-false-positive tool."]
    (out / "RESULTS.md").write_text("\n".join(md) + "\n", encoding="utf-8")
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
