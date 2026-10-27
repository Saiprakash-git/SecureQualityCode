#!/usr/bin/env python3
"""Cohort-level comparison: our five-model benchmark vs the base paper's published
outputs, same 40 tasks, C / C++ / Python only (BASE_COMPARISON_PROTOCOL.md, approved
with Java excluded). Descriptive and secondary: differences belong to COHORTS
(models + prompt + temperature + token limits + extraction + dates), not to models.
Scanner hits are potential findings, not confirmed vulnerabilities.

Inputs (read-only):
  ours: results/benchmark-40-final-consolidated-v2/combined_per_combination.csv
  base: results/base-40-published-v1/{gate1,scans}
Writes: results/base-40-comparison-v1/

Analyses (pre-declared):
  primary          all 600 base files vs our 600 planned C/C++/Python cells
  S1_identical     base: only files classed 'identical' to the base test copies
  S2_no_escape     base: excluding C/C++ files with a doubled-backslash escape
  S3_blocked_excl  both cohorts: 'blocked_dependency' removed from the gate-1 denominator
  S4_symmetric_integrity  POST-HOC (not pre-declared): base files have no finish_reason, so
                   they can never fail the 'truncated_finish_length' integrity check. Here
                   our files are re-graded from their recorded gate-1 fields with that one
                   check dropped (source-observable checks only, same precedence as
                   evaluate.gate1_status); base statuses are unchanged.
Gate 1 pass rate uses planned cells (a failed generation counts as not passing);
'artifact' rates use files with an artifact. Quality/security use gate-1-passing files.
Paired check: per (task, language) cell, models passing (0-5) in each cohort,
Wilcoxon signed-rank (scipy, zero_method='wilcox', two-sided, normal approximation with
tie correction since scipy picks 'approx' when ties/zeros are present)."""
import csv
import json
import re
from collections import Counter, defaultdict
from pathlib import Path
import sys

EVAL = Path(__file__).resolve().parents[1]
ROOT = EVAL.parent
sys.path.insert(0, str(EVAL))
from consolidate import lizard_by_artifact, _wilson_raw  # noqa: E402
from scipy.stats import wilcoxon  # noqa: E402

LANGS = ["c", "cpp", "python"]
DOUBLE = re.compile(r'\\\\[0nt"\']')  # same pattern as count_escape_artifacts.py
OURS = EVAL / "results/benchmark-40-final-consolidated-v2/combined_per_combination.csv"
BASE = EVAL / "results/base-40-published-v1"
OUT = EVAL / "results/base-40-comparison-v1"
OURS_G1 = EVAL / "results/benchmark-40-final-gate1/gate1_results.json"


def symmetric_status(g):
    """evaluate.gate1_status precedence without the ledger-only truncation check."""
    if g["gate1_status"] == "no_artifact":
        return "no_artifact"
    if [f for f in g["integrity_failures"].split(";") if f and f != "truncated_finish_length"]:
        return "fail_integrity"
    if g["build_status"] == "toolchain_error":
        return "toolchain_error"
    if g["syntax_status"] == "error":
        return "fail_syntax"
    if g["build_status"] == "blocked_dependency":
        return "blocked_dependency"
    if g["build_status"] == "error":
        return "fail_compile"
    return "pass"


def load(path):
    with open(path, encoding="utf-8", newline="") as fh:
        return list(csv.DictReader(fh))


def base_rows():
    inv = {r["combo_index"]: r for r in json.loads((BASE / "gate1/inventory.json").read_text(encoding="utf-8"))["rows"]}
    g1 = {r["combo_index"]: r for r in json.loads((BASE / "gate1/gate1_results.json").read_text(encoding="utf-8"))}
    scan = {int(r["combo_index"]): r for r in load(BASE / "scans/artifact_scan_summary.csv")}
    liz = lizard_by_artifact(BASE / "scans/raw/lizard.csv")
    rows = []
    for c, r in sorted(inv.items()):
        s = scan[c]
        sum_ccn, fn_nloc, _ = liz.get(c, (0, 0, 0))
        text = (ROOT / r["published_path"]).read_text(encoding="utf-8", errors="replace")
        rows.append({"cohort": "base", "combo_index": c, "task_id": r["task_id"], "language": r["language"],
                     "model": r["model_label"], "has_artifact": True, "gate1_status": g1[c]["gate1_status"],
                     "nloc": int(s["nloc"] or 0), "sum_ccn": sum_ccn,
                     "ccd": round(sum_ccn / fn_nloc, 4) if fn_nloc else "", "avg_ccn": s["avg_ccn"],
                     "cppcheck_findings": int(s["cppcheck_findings"] or 0),
                     "sec_high": int(s["sec_high"] or 0), "sec_medium": int(s["sec_medium"] or 0),
                     "gate1_symmetric": g1[c]["gate1_status"], "base_class": r["base_class"],
                     "escape_pattern": r["language"] in ("c", "cpp") and bool(DOUBLE.search(text)),
                     "source": r["published_path"]})
    return rows


def our_rows():
    g1 = {g["combo_index"]: g for g in json.loads(OURS_G1.read_text(encoding="utf-8"))}
    rows = []
    for r in load(OURS):
        if r["language"] not in LANGS:
            continue
        rows.append({"cohort": "ours", "combo_index": int(r["combo_index"]), "task_id": int(r["task_id"]),
                     "language": r["language"], "model": r["model_label"],
                     "has_artifact": r["generation_status"] in ("generated", "generated_truncated"),
                     "gate1_status": r["gate1_status"], "nloc": int(r["nloc"] or 0),
                     "sum_ccn": int(r["sum_ccn"] or 0), "ccd": r["ccd"], "avg_ccn": r["avg_ccn"],
                     "cppcheck_findings": int(r["cppcheck_findings"] or 0),
                     "sec_high": int(r["sec_high"] or 0), "sec_medium": int(r["sec_medium"] or 0),
                     "gate1_symmetric": symmetric_status(g1[int(r["combo_index"])]), "base_class": "", "escape_pattern": False, "source": f"combo {r['combo_index']}"})
    return rows


def rate(k, n):
    lo, hi = _wilson_raw(k, n) if n else (None, None)
    return {"k": k, "n": n, "rate": k / n if n else None, "lo": lo, "hi": hi}


def newcombe_unpaired(a, b):
    """Newcombe (1998) method 10 for p1 - p2 from independent samples (Wilson hybrid score)."""
    if not a["n"] or not b["n"]:
        return None, None, None
    d = a["rate"] - b["rate"]
    lo = d - ((a["rate"] - a["lo"]) ** 2 + (b["hi"] - b["rate"]) ** 2) ** 0.5
    hi = d + ((a["hi"] - a["rate"]) ** 2 + (b["rate"] - b["lo"]) ** 2) ** 0.5
    return d, lo, hi


def measures(sel, blocked_excluded=False, key="gate1_status"):
    sel = [dict(r, gate1_status=r[key]) for r in sel]
    denom = [r for r in sel if not (blocked_excluded and r["gate1_status"] == "blocked_dependency")]
    arts = [r for r in denom if r["has_artifact"]]
    g1 = [r for r in denom if r["gate1_status"] == "pass"]
    nloc = sum(r["nloc"] for r in g1)
    cc = [r for r in g1 if r["language"] in ("c", "cpp")]
    nloc_cc = sum(r["nloc"] for r in cc)
    ccn = [float(r["avg_ccn"]) for r in g1 if r["avg_ccn"] not in ("", None)]
    with_fn = [r for r in g1 if r["ccd"] not in ("", None)]
    return {
        "planned": len(denom), "artifacts": len(arts),
        "gate1_counts": dict(Counter(r["gate1_status"] for r in denom)),
        "gate1_pass_planned": rate(len(g1), len(denom)),
        "gate1_pass_of_artifacts": rate(len(g1), len(arts)),
        "sec_medhigh_of_pass": rate(sum(1 for r in g1 if r["sec_high"] + r["sec_medium"] > 0), len(g1)),
        "mean_nloc": nloc / len(g1) if g1 else None,
        "mean_avg_ccn": sum(ccn) / len(ccn) if ccn else None,
        "mean_ccd": sum(float(r["ccd"]) for r in with_fn) / len(with_fn) if with_fn else None,
        "pooled_ccd": sum(r["sum_ccn"] for r in with_fn) / nloc if nloc else None,
        "cppcheck_per_kloc_c_cpp": 1000 * sum(r["cppcheck_findings"] for r in cc) / nloc_cc if nloc_cc else None,
        "nloc_total": nloc, "nloc_c_cpp": nloc_cc,
    }


ANALYSES = {
    "primary": (lambda r: True, False, "gate1_status"),
    "S1_identical": (lambda r: r["cohort"] == "ours" or r["base_class"] == "identical", False, "gate1_status"),
    "S2_no_escape": (lambda r: r["cohort"] == "ours" or not r["escape_pattern"], False, "gate1_status"),
    "S3_blocked_excl": (lambda r: True, True, "gate1_status"),
    "S4_symmetric_integrity": (lambda r: True, False, "gate1_symmetric"),
}
RATES = ["gate1_pass_planned", "gate1_pass_of_artifacts", "sec_medhigh_of_pass"]
MEANS = ["mean_nloc", "mean_avg_ccn", "mean_ccd", "pooled_ccd", "cppcheck_per_kloc_c_cpp"]


def paired_cells(rows, key="gate1_status"):
    cnt = defaultdict(lambda: {"ours": 0, "base": 0})
    for r in rows:
        if r[key] == "pass":
            cnt[(r["task_id"], r["language"])][r["cohort"]] += 1
        else:
            cnt[(r["task_id"], r["language"])]
    out = {}
    for lang in LANGS + ["all"]:
        keys = sorted(k for k in cnt if lang == "all" or k[1] == lang)
        d = [cnt[k]["ours"] - cnt[k]["base"] for k in keys]
        res = wilcoxon(d, zero_method="wilcox") if any(d) else None
        out[lang] = {"cells": len(keys), "ours_higher": sum(x > 0 for x in d), "equal": sum(x == 0 for x in d),
                     "base_higher": sum(x < 0 for x in d), "mean_diff": sum(d) / len(d),
                     "median_diff": sorted(d)[len(d) // 2] if len(d) % 2 else (sorted(d)[len(d) // 2 - 1] + sorted(d)[len(d) // 2]) / 2,
                     "wilcoxon_W": float(res.statistic) if res else None, "p": float(res.pvalue) if res else None}
    return out, cnt


def r4(x):
    return "" if x is None else round(x, 4)


def main():
    if OUT.exists() and any(OUT.iterdir()):
        raise SystemExit(f"Output folder already has files: {OUT}")
    OUT.mkdir(parents=True, exist_ok=True)
    ours, base = our_rows(), base_rows()
    assert len(ours) == 600 and len(base) == 600, (len(ours), len(base))
    allrows = ours + base
    with (OUT / "cohort_rows.csv").open("w", encoding="utf-8", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(base[0]), lineterminator="\n")
        w.writeheader()
        w.writerows(allrows)

    result = {"protocol": "evaluation/base_compare/BASE_COMPARISON_PROTOCOL.md", "secondary_analysis": True,
              "base_snapshot_id": json.loads((BASE / "SNAPSHOT_base-40-published.json").read_text(encoding="utf-8"))["snapshot_id"],
              "base_files": len(base), "base_identical": sum(r["base_class"] == "identical" for r in base),
              "base_escape_pattern": sum(r["escape_pattern"] for r in base), "analyses": {}}
    table = []
    for name, (keep, bex, key) in ANALYSES.items():
        sel = [r for r in allrows if keep(r)]
        res = {}
        for lang in LANGS + ["all"]:
            res[lang] = {}
            for coh in ("ours", "base"):
                res[lang][coh] = measures([r for r in sel if r["cohort"] == coh and (lang == "all" or r["language"] == lang)], bex, key)
            for m in RATES:
                d, lo, hi = newcombe_unpaired(res[lang]["ours"][m], res[lang]["base"][m])
                res[lang][f"diff_{m}"] = {"diff": d, "lo": lo, "hi": hi}
                o, b = res[lang]["ours"][m], res[lang]["base"][m]
                table.append([name, lang, m, o["k"], o["n"], r4(o["rate"]), r4(o["lo"]), r4(o["hi"]),
                              b["k"], b["n"], r4(b["rate"]), r4(b["lo"]), r4(b["hi"]), r4(d), r4(lo), r4(hi)])
            for m in MEANS:
                o, b = res[lang]["ours"][m], res[lang]["base"][m]
                table.append([name, lang, m, "", "", r4(o), "", "", "", "", r4(b), "", "",
                              r4(o - b) if o is not None and b is not None else "", "", ""])
        res["paired_cells"] = paired_cells(sel, key)[0] if name in ("primary", "S4_symmetric_integrity") else None
        result["analyses"][name] = res
    with (OUT / "cohort_comparison.csv").open("w", encoding="utf-8", newline="") as fh:
        w = csv.writer(fh, lineterminator="\n")
        w.writerow(["analysis", "language", "measure", "ours_k", "ours_n", "ours_value", "ours_ci_lo", "ours_ci_hi",
                    "base_k", "base_n", "base_value", "base_ci_lo", "base_ci_hi", "diff_ours_minus_base",
                    "diff_ci_lo", "diff_ci_hi"])
        w.writerows(table)
    pc, cnt = paired_cells(allrows)
    with (OUT / "paired_cells.csv").open("w", encoding="utf-8", newline="") as fh:
        w = csv.writer(fh, lineterminator="\n")
        w.writerow(["task_id", "language", "ours_models_passing", "base_models_passing", "diff"])
        for k in sorted(cnt):
            w.writerow([k[0], k[1], cnt[k]["ours"], cnt[k]["base"], cnt[k]["ours"] - cnt[k]["base"]])
    (OUT / "comparison.json").write_text(json.dumps(result, indent=1, default=str) + "\n", encoding="utf-8")
    p = result["analyses"]["primary"]
    for lang in LANGS + ["all"]:
        o, b = p[lang]["ours"]["gate1_pass_planned"], p[lang]["base"]["gate1_pass_planned"]
        print(f"{lang:6s} gate1 ours {o['k']}/{o['n']}  base {b['k']}/{b['n']}  wilcoxon p={pc[lang]['p']}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
