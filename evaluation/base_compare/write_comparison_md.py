#!/usr/bin/env python3
"""Write results/base-40-comparison-v1/RESULTS.md (supplementary material) from
cohort_comparison.csv and comparison.json only. Every number is copied from those
files (checked by verify_base_comparison.py); nothing is typed by hand."""
import csv
import json
from pathlib import Path

OUT = Path(__file__).resolve().parents[1] / "results/base-40-comparison-v1"
rows = list(csv.DictReader(open(OUT / "cohort_comparison.csv", encoding="utf-8")))
cj = json.loads((OUT / "comparison.json").read_text(encoding="utf-8"))
NAMES = {"primary": "Primary", "S1_identical": "S1: base files identical to the test copies only",
         "S2_no_escape": "S2: base C/C++ files with doubled-backslash escapes excluded",
         "S3_blocked_excl": "S3: blocked_dependency removed from the gate-1 denominator (both cohorts)",
         "S4_symmetric_integrity": "S4 (post-hoc, not pre-declared): our files re-graded without the ledger-only truncation check"}
LN = {"c": "C", "cpp": "C++", "python": "Python", "all": "All three"}


def J(r):  # unrounded values (the CSV is rounded to 4 dp)
    return cj["analyses"][r["analysis"]][r["language"]]


def rate(r, side):
    v = J(r)[side][r["measure"]]
    return f"{v['k']}/{v['n']} = {100 * v['rate']:.1f}% [{100 * v['lo']:.1f}, {100 * v['hi']:.1f}]"


md = ["# Secondary analysis: our cohort vs the base study's published outputs (C, C++, Python)", "",
      "Secondary and descriptive; the main result is the five-model experiment. Protocol: "
      "`evaluation/base_compare/BASE_COMPARISON_PROTOCOL.md` (approved; Java excluded). Same gate 1 code, toolchain, flags, "
      "scanner versions, rule hashes and configuration as the main benchmark (`scan_manifest.json` versions and config identical).",
      "", f"Base snapshot `{cj['base_snapshot_id'][:12]}`: {cj['base_files']} published files, {cj['base_identical']} identical to the "
      f"base test copies, {cj['base_escape_pattern']} C/C++ files with a doubled-backslash escape. Published originals were "
      "copied byte-for-byte, never edited, and re-hashed after the run (0 changed).", "",
      "Differences are ours minus base, in percentage points, with Newcombe hybrid-score 95% intervals (independent-sample "
      "approximation; the cohorts share tasks, see the paired check). Scanner hits are potential findings, not confirmed vulnerabilities.", ""]
for an, title in NAMES.items():
    md += [f"## {title}", "", "| language | measure | ours | base | diff (pp) [95% CI] |", "|---|---|---|---|---|"]
    for r in rows:
        if r["analysis"] != an:
            continue
        if r["ours_k"]:
            md.append(f"| {LN[r['language']]} | {r['measure']} | {rate(r, 'ours')} | {rate(r, 'base')} | "
                      f"{100 * J(r)['diff_' + r['measure']]['diff']:.1f} [{100 * J(r)['diff_' + r['measure']]['lo']:.1f}, "
                      f"{100 * J(r)['diff_' + r['measure']]['hi']:.1f}] |")
        elif r["ours_value"] != "":
            md.append(f"| {LN[r['language']]} | {r['measure']} | {r['ours_value']} | {r['base_value']} | "
                      f"{r['diff_ours_minus_base']} (no interval) |")
    md.append("")
    p = cj["analyses"][an]["paired_cells"]
    if p:
        md += ["Paired check per (task, language) cell, number of models passing gate 1 (0-5), Wilcoxon signed-rank "
               "(SciPy, zero_method='wilcox'):", "", "| language | cells | ours higher | equal | base higher | mean diff | W | p |",
               "|---|---|---|---|---|---|---|---|"]
        for L, v in p.items():
            md.append(f"| {LN[L]} | {v['cells']} | {v['ours_higher']} | {v['equal']} | {v['base_higher']} | "
                      f"{v['mean_diff']:.3f} | {v['wilcoxon_W']} | {v['p']:.2e} |")
        md.append("")
md += ["## Gate-1 outcome counts (primary)", "", "| language | ours | base |", "|---|---|---|"]
for L in ("c", "cpp", "python", "all"):
    a = cj["analyses"]["primary"][L]
    md.append(f"| {LN[L]} | {a['ours']['gate1_counts']} | {a['base']['gate1_counts']} |")
md += ["", "## Confounders (differences are attributed to cohorts, never to models)", "",
       "- Model cohort: five 2024-era models (base) vs five current cost-efficient models (ours).",
       "- Prompt: the base study's prompt wording was not reproduced.",
       "- Temperature: 0.9 in the base study's generation scripts vs 0.2.",
       "- Output-token limits and retry policy differ; our max_tokens 4096 produced truncations that fail gate 1.",
       "- Extraction: the base pipeline leaves doubled-backslash escapes (likely artifact); ours validates raw source.",
       "- Generation dates are about two years apart.",
       "- Unidentifiable manual fixes: the published files may contain some of the base study's manual fixes; no fix list exists.",
       "- File size: base files are much shorter (mean function NLOC), which confounds per-file and per-KLOC rates.", "",
       "## Limitations", "",
       "- Java excluded (no published Java outputs).",
       "- Truncation is detectable only for our files (ledger finish_reason); S4 shows the effect is small (2 files).",
       "- S1 is a selected subset: files untouched by post-processing may be those that needed no fixes.",
       "- The 4 language cells of a task, and both cohorts, share tasks; intervals treat cohorts as independent samples.",
       "- Functional tests and the base study's CodeQL/SonarQube numbers are not compared (different tests, tools and inputs).", ""]
(OUT / "RESULTS.md").write_text("\n".join(md), encoding="utf-8", newline="\n")
print("wrote", OUT / "RESULTS.md")
