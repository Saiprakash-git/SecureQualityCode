# Metric definitions (consolidate.py version 2)

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
