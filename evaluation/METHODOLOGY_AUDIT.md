# Methodology audit: our benchmark-40 evaluation vs. the base paper (Kharma et al., TDSC 2026)

Evidence only: scripts, manifests, raw tool outputs and result files. No API calls were made and no result was modified. The base-paper methodology was read from `Security_and_Quality_in_LLM-Generated_Code_A_Multi-Language_Multi-Model_Analysis.pdf` (Sec. III-C/E, pp. 7303–7305).

## 1. Tools that actually ran

| Tool / step | Evidence of execution | Artifacts covered | Configuration (from the evidence) |
|---|---|---|---|
| Inventory + gate 1 (`evaluation/evaluate.py`) | `results/benchmark-40-final-gate1/eval_manifest.json` (generated 2026-10-26T23:25:57, evaluator m1.0, sha 523d84e0…), `gate1_results.csv/json`, `diagnostics/` | 800 rows; 734 artifacts checked | Python 3.11.1 `ast.parse`+`compile` (no import resolution); javac 21.0.6 `-proc:none -implicit:none`, empty class path, plus a parse-only JDK API helper; clang 20.1.2 (zig 0.15.2) `-target x86_64-linux-gnu -std=gnu17/gnu++17 -Wall -c`, no link; 120 s timeout |
| Functional tests (`evaluation/functional.py plan --execute`) | `results/benchmark-40-final-functional-v2/functional_plan.csv`, `sandbox_self_test.txt` (31 CHECK lines, 0 FAIL), `workspaces/*/sandbox_output.txt`, Java `reports/*.xml` | 130 executed (all passed); 670 not testable, with reasons | WSL2 Ubuntu 24.04 + bubblewrap 0.9.0 + seccomp; gcc 13.3, Python 3.12.3, OpenJDK 21.0.12, JUnit 1.11.4 (`evaluation/sandbox/SANDBOX_STATE.md`). The v1 run (`…-functional/`) is SUPERSEDED |
| Semgrep 1.182.1 | `results/benchmark-40-final-scans/raw/semgrep.json` (`paths.scanned` = 734), `semgrep.log` | 734/734 (py 187, java 189, c 175, cpp 183); 39 parse errors, all on gate-1 failures | `--config cwe-top-25.yml --config security-audit.yml` (sha256 725e296a…, 83d7cde9… in `raw/versions.txt`), `--timeout 30 --metrics=off` |
| Bandit 1.9.4 | `raw/bandit.json` (`metrics`: 187 files, 7,900 LOC; 51 results) | 187/187 Python | `-r src/python`, all severities and confidences |
| Flawfinder 2.0.20 | `raw/flawfinder.csv` (1,284 hit rows), `flawfinder.log` (parse warnings on truncated files) | Run on all 358 C/C++ files (`run_scanners.sh`); 175 files have ≥1 hit. `--csv` suppresses the "files analysed" summary, so per-file coverage is evidenced only by the command | `--csv --minlevel=0` |
| Cppcheck 2.13.0 | `raw/cppcheck_c.xml` (86 errors), `raw/cppcheck_cpp.xml` (106); version tag 2.13.0 in the XML | All 358 C/C++ files (command). The XML lists findings only, not checked files | `--enable=warning,style,performance,portability --inline-suppr`, `--std=c17` / `--std=c++17` |
| Lizard 1.24.3 | `raw/lizard.csv` (3,084 function rows) | 714/734 artifacts have ≥1 function; 20 have none (top-level scripts etc.) | `-l python -l java -l cpp --csv` |
| Normalisation and aggregation | `scan.py` → `findings.csv` (1,600), `artifact_scan_summary.csv`, `scan_manifest.json`; `consolidate.py` → `benchmark-40-final-consolidated/*` | 734 / 800 | Severity mapping recorded in `scan_manifest.json` |
| Triage sample | `results/benchmark-40-final-scans/triage_sample.csv` (65 rows, seed 20261027); AI-proposed labels in `paper/data/triage_ai_proposed.csv` | 25 high + 40 medium findings | Not human-confirmed |

## 2. Code-quality metrics produced

- **Lizard (per function, raw):** NLOC, CCN, token count, parameter count, length, location.
- **Aggregated per artifact** (`artifact_scan_summary.csv`): `nloc` (sum of function NLOC; top-level code is not counted), `functions`, `avg_ccn`, `max_ccn`, `functions_ccn_gt10`. Token count, parameter count and length are **not** aggregated.
- **Cppcheck per artifact:** `cppcheck_findings`, `cppcheck_errors` (C/C++ only).
- **By model and by model × language:** `consolidated/summary.json` (`by_model`, `by_language`, `by_model_language`) holds `mean_nloc`, `mean_avg_ccn`, `functions_ccn_gt10`, `cppcheck_per_kloc`.
  - `table_by_model.csv` has mean_nloc, mean_avg_ccn and cppcheck_per_kloc.
  - `table_by_model_language.csv` has mean_avg_ccn and cppcheck_per_kloc, but **not** mean_nloc or functions_ccn_gt10.
  - The paper's Table XI is per model only.
- **Defect found:** `cppcheck_per_kloc` (`consolidate.py:148`) divides C/C++ findings by the NLOC of **all languages**, which understates C/C++ density about 2.2×. Recomputed over C/C++ only:

  | Model | Reported | C/C++ only |
  |---|---|---|
  | Claude Haiku 5.5 | 2.38 | 5.21 |
  | GPT-6 Luna | 1.69 | 3.69 |
  | Gemini 3.8 Flash | 2.56 | 5.56 |
  | Mistral Large 4 | 9.66 | 21.27 |
  | Qwen3.8 Flash | 3.48 | 7.68 |

  The ordering is unchanged; Table XI's absolute values are wrong as labelled.
- **Not produced:** cognitive complexity; cyclomatic complexity density (derivable as CCN/NLOC, not computed); maintainability or code smells for Python/Java (no Python/Java linter ran); reliability bugs (SonarQube); security hotspots.

## 3. CodeQL and SonarQube

| | CodeQL | SonarQube |
|---|---|---|
| Installed on host or in eval-sandbox | **No** (`codeql` not on PATH; no install directory; not in `/opt` of eval-sandbox) | **No** (`sonar-scanner` not found; no Docker or Podman) |
| Scripts in the repo | Base-paper artifact only: `Repo/…/python/analysis/codeQL/extract_codeql_vulns.py` | Base-paper artifact only: `compose-sonarqube.yaml`, `sonar-project.properties` (sources `./dataset/output`), `python/analysis/sonarqube/download_report.py` (SonarCloud API) |
| Existing reports | `Repo/…/python/analysis/codeQL/*.xlsx`, `*-compare CodeQL and SonarQube.xlsx`: **base-paper models only** (CLAUDE/GEMINI/OPENAI/MISTRAL/PERPLEXITY strings; none of our five) | `Repo/…/sonarqube/sonarcloud_report_mohkharma_*_202511xx-detailed.xlsx`: base-paper models only |
| Executed on our benchmark | **No.** Our code and results mention them only in disclaimers (`RESULTS.md`, paper) and plans (`runner/README.md`, `PILOT_REFRESH_NOTES.md`) | **No** |

## 4. Base-paper steps: reproduced / adapted / omitted / incomplete

| Base-paper step | Our status | Evidence / difference |
|---|---|---|
| 200 manually curated tasks, 7 categories | **Adapted:** 40 tasks across 7 strata, chosen by judgment | `runner/BENCHMARK_40_SELECTION.md` |
| 5 LLM families (Claude 3.5, Gemini 1.5, GPT-4o, Codestral, Llama-3-Sonar) | **Adapted:** 5 current cost-efficient models via OpenRouter | `runner/run_config_benchmark_40task*.json` |
| Prompt wording | **Not reproduced:** our own system and user instruction | `runner/run_experiment.py` `build_instruction`, system prompt |
| Temperature 0.9 plus a lower-temperature ablation | **Adapted / omitted:** 0.2, top-p 0.95, single setting, no ablation | config files |
| Environment: Debian 12, OpenJDK 17.0.8, Python 3.11.9, CMake 3.28.6 (C++17) | **Adapted:** gate 1 uses javac 21.0.6, Python 3.11.1, clang 20.1.2 gnu17/gnu++17 (compile only); the sandbox uses gcc 13.3, Python 3.12.3, OpenJDK 21 | `eval_manifest.json`, `SANDBOX_STATE.md` |
| Manual fix protocol (73/4,000 files edited to compile) | **Omitted by design:** no edits; truncated outputs count as failures; missing third-party headers are "blocked" | `gate1_results.csv` |
| Compilation success rate | **Reproduced, adapted** (integrity + syntax + compile, no linking, blocked category) | `benchmark-40-final-gate1/summary.json` |
| Per-sample unit tests (4,000 files, score 0–100%) | **Partially reproduced:** the base paper's own per-model tests were recovered and audited (36/76 approved), with name-only adapters; 130 executed, binary pass/fail. Per-test partial scores were not computed | `functional/oracle_review.json`, `adapter_review.json`, `…-functional-v2/` |
| Code coverage (JaCoCo etc. on a subset) | **Omitted** | — |
| LoC, cyclomatic complexity | **Reproduced with a different tool** (Lizard function NLOC and CCN) | `artifact_scan_summary.csv` |
| CC density, cognitive complexity | **Omitted** (CCD derivable; CoC needs SonarQube or another tool) | — |
| SonarQube reliability, maintainability, security hotspots | **Omitted.** Cppcheck covers C/C++ partly; nothing covers Python/Java quality | — |
| CodeQL vulnerabilities | **Omitted;** replaced by Semgrep, Bandit and Flawfinder | `raw/` |
| Manual review of all vulnerabilities + stratified 20% validation | **Incomplete:** AI-assisted triage of a 65-finding sample, not human-confirmed | `paper/data/triage_ai_proposed.csv` |
| CWE grouping of findings | **Partially:** scanner CWE labels aggregated (top CWEs) | `consolidated/summary.json` `top_cwes_artifacts` |

## 5. Do the results support the research questions and model comparisons?

| Question | Supported? | Basis |
|---|---|---|
| Paper RQ1 (build reliability by model and language) | **Yes.** All 800 combinations; Wilson CIs; **paired gate-1 McNemar tests exist but are not reported in the paper** (`benchmark-40-final-gate1/summary.json` → `paired`, 160/160 complete cells). Holm-corrected, 7 of 10 pairs are significant: Luna > Haiku, Mistral, Qwen; Gemini > Haiku, Qwen; Haiku > Qwen; Mistral > Qwen. Luna vs Gemini p=0.55, Haiku vs Mistral p=0.56, Gemini vs Mistral p=0.049 unadjusted, not significant after correction | |
| Paper RQ2 (functional) | **Partially.** 130/800 (16.2%) executed, mostly kata tasks, all passed. No model separation and no security-task evidence | `…-functional-v2/` |
| Paper RQ3 (security) | **Partially.** Potential findings with CIs across 734 artifacts; model differences overlap (CIs in Table VIII); language effect strong. No human-confirmed precision, no CodeQL, so no like-for-like comparison with the base paper | `consolidated/summary.json` |
| Paper RQ4 (quality) | **Partially.** Lizard NLOC/CCN for all languages; Cppcheck C/C++ only; per-KLOC defect above; no maintainability or smells for Python/Java | as above |
| Paper RQ5 (cost) | **Yes.** Ledger costs per combination; excludes $0.0655 provisional and $0.13 unreconciled spend | `combined_per_combination.csv` `cost_usd_total` |
| Draft's "compared with the base study" | **Only qualitatively.** Different models, prompt, temperature, tools and no manual fixes; no numeric comparison is valid yet | — |
| `research-agenda.md` central question (closed-loop repair system) | **Not supported:** no such system was built or evaluated | — |

## 6. Smallest additions that would materially strengthen the paper

All but item 5 need no API calls. Ordered by value per effort:

1. **Fix the Cppcheck density denominator** in a new consolidated folder, then regenerate the tables. Minutes; corrects a published number.
2. **Report the existing paired gate-1 McNemar tests with Holm correction.** No new computation; turns RQ1 into a statistically supported model comparison.
3. **Run the same tools on the base paper's own outputs for the same 40 tasks.** C/C++/Python are in `Repo/…/dataset/output/` (5 models × 40 tasks); Java is in `evaluation/.upstream/llm-generated-code-java/src/main/java` (200 files). Gate 1 plus the identical scanners give a like-for-like old-vs-new comparison under one pipeline. Roughly 1–2 hours, no API calls. Caveat: the base outputs include their 73 manual compile fixes.
4. **Human confirmation of the 65 triage labels** (`human_label` column). About 1–2 hours of reviewer time; turns "AI-proposed" into a measured precision per tool.
5. **CodeQL on the 604 gate-1-passing artifacts** (and optionally on the base outputs). This is the base paper's security tool and closes the main comparability gap. It needs the CodeQL CLI and query packs (external download, about 1–2 GB, permission required) and builds for C/C++ inside the sandbox. Several hours. SonarQube needs a server (Docker or SonarCloud upload) and is the costlier option.
6. **Cheap quality completions from existing data:** CC density (CCN/NLOC) and a per-language quality table from `summary.json`. Minutes.

Not minimal and therefore deferred: temperature ablation (API calls), new interface-fixed tests for security tasks, code coverage, Python/Java linters.
