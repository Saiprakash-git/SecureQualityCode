# Benchmark-40 evaluation results

Snapshot `30d64f1cb11b3160…`: 800 planned combinations (40 tasks × 5 models × 4 languages). Rates carry Wilson 95% intervals.

**How to read this.** Gate 1 and executed functional tests are confirmed results. Static security findings are *potential* issues reported by scanners; they are not verified vulnerabilities, and an artifact with no findings is not shown to be secure. Untested and blocked combinations are counted separately and are not treated as failures or passes.

## 1. Generation, gate 1 and cost by model

| model | artifacts / 160 | truncated | gate 1 pass | cost (USD) | cost per gate-1 pass |
|---|---|---|---|---|---|
| claude-haiku-5-5 | 159 | 26 | 118/160 = 73.8% [66.4–80.0] | 0.1599 | 0.001355 |
| gemini-3-8-flash | 156 | 14 | 131/160 = 81.9% [75.2–87.1] | 1.1980 | 0.009145 |
| gpt-6-luna | 160 | 11 | 134/160 = 83.8% [77.2–88.7] | 0.1026 | 0.000766 |
| mistral-large-4 | 149 | 19 | 122/160 = 76.2% [69.1–82.2] | 2.4555 | 0.020127 |
| qwen3-8-flash | 110 | 10 | 99/160 = 61.9% [54.1–69.0] | 0.4580 | 0.004627 |

## 2. Gate 1 pass by model and language

| model | python | java | c | cpp |
|---|---|---|---|---|
| claude-haiku-5-5 | 39/40 = 97.5% [87.1–99.6] | 31/40 = 77.5% [62.5–87.7] | 25/40 = 62.5% [47.0–75.8] | 23/40 = 57.5% [42.2–71.5] |
| gemini-3-8-flash | 39/40 = 97.5% [87.1–99.6] | 38/40 = 95.0% [83.5–98.6] | 25/40 = 62.5% [47.0–75.8] | 29/40 = 72.5% [57.2–83.9] |
| gpt-6-luna | 40/40 = 100.0% [91.2–100.0] | 37/40 = 92.5% [80.1–97.4] | 27/40 = 67.5% [52.0–79.9] | 30/40 = 75.0% [59.8–85.8] |
| mistral-large-4 | 37/40 = 92.5% [80.1–97.4] | 35/40 = 87.5% [73.9–94.5] | 23/40 = 57.5% [42.2–71.5] | 27/40 = 67.5% [52.0–79.9] |
| qwen3-8-flash | 31/40 = 77.5% [62.5–87.7] | 29/40 = 72.5% [57.2–83.9] | 17/40 = 42.5% [28.5–57.8] | 22/40 = 55.0% [39.8–69.3] |

## 3. Functional tests (executed in the verified sandbox, approved oracles and adapters only)

| model | executed | passed | failed | error | timeout | pass rate of executed |
|---|---|---|---|---|---|---|
| claude-haiku-5-5 | 28 | 28 | 0 | 0 | 0 | 28/28 = 100.0% [87.9–100.0] |
| gemini-3-8-flash | 26 | 26 | 0 | 0 | 0 | 26/26 = 100.0% [87.1–100.0] |
| gpt-6-luna | 24 | 24 | 0 | 0 | 0 | 24/24 = 100.0% [86.2–100.0] |
| mistral-large-4 | 26 | 26 | 0 | 0 | 0 | 26/26 = 100.0% [87.1–100.0] |
| qwen3-8-flash | 26 | 26 | 0 | 0 | 0 | 26/26 = 100.0% [87.1–100.0] |

Functional results by language (pass / executed):

| model | python | java | c | cpp |
|---|---|---|---|---|
| claude-haiku-5-5 | 8/8 | 7/7 | 6/6 | 7/7 |
| gemini-3-8-flash | 8/8 | 6/6 | 5/5 | 7/7 |
| gpt-6-luna | 8/8 | 5/5 | 4/4 | 7/7 |
| mistral-large-4 | 8/8 | 8/8 | 3/3 | 7/7 |
| qwen3-8-flash | 8/8 | 7/7 | 4/4 | 7/7 |

Why combinations were not functionally tested (all models):

- no_oracle: 268
- gate1: 196
- oracle_rejected: 162
- adapter: 44

## 4. Potential security findings (static scanners, gate-1-passing artifacts)

Headline metric: share of artifacts with at least one medium- or high-severity potential finding. Low-severity hits (e.g. flawfinder level 0-1 printf with constant formats, bandit assert_used) are kept in findings.csv but not headlined.

| model | scanned | ≥1 medium/high | ≥1 high | ≥1 any (incl. low) | medium+high / KLOC | semgrep | bandit | flawfinder |
|---|---|---|---|---|---|---|---|---|
| claude-haiku-5-5 | 118 | 21/118 = 17.8% [11.9–25.7] | 3/118 = 2.5% [0.9–7.2] | 35/118 = 29.7% [22.2–38.4] | 9.0 | 11 | 11 | 186 |
| gemini-3-8-flash | 131 | 18/131 = 13.7% [8.9–20.7] | 4/131 = 3.0% [1.2–7.6] | 37/131 = 28.2% [21.2–36.5] | 5.66 | 10 | 28 | 184 |
| gpt-6-luna | 134 | 24/134 = 17.9% [12.3–25.3] | 5/134 = 3.7% [1.6–8.4] | 31/134 = 23.1% [16.8–31.0] | 7.85 | 13 | 6 | 109 |
| mistral-large-4 | 122 | 21/122 = 17.2% [11.5–24.9] | 6/122 = 4.9% [2.3–10.3] | 28/122 = 22.9% [16.4–31.2] | 10.81 | 14 | 5 | 131 |
| qwen3-8-flash | 99 | 12/99 = 12.1% [7.1–20.0] | 2/99 = 2.0% [0.6–7.1] | 16/99 = 16.2% [10.2–24.6] | 6.38 | 7 | 1 | 64 |

By language (≥1 medium/high finding among scanned gate-1-passing artifacts):

| model | python | java | c | cpp |
|---|---|---|---|---|
| claude-haiku-5-5 | 3/39 = 7.7% [2.6–20.3] | 1/31 = 3.2% [0.6–16.2] | 15/25 = 60.0% [40.7–76.6] | 2/23 = 8.7% [2.4–26.8] |
| gemini-3-8-flash | 2/39 = 5.1% [1.4–16.9] | 3/38 = 7.9% [2.7–20.8] | 12/25 = 48.0% [30.0–66.5] | 1/29 = 3.5% [0.6–17.2] |
| gpt-6-luna | 2/40 = 5.0% [1.4–16.5] | 2/37 = 5.4% [1.5–17.7] | 16/27 = 59.3% [40.7–75.5] | 4/30 = 13.3% [5.3–29.7] |
| mistral-large-4 | 3/37 = 8.1% [2.8–21.3] | 3/35 = 8.6% [3.0–22.4] | 13/23 = 56.5% [36.8–74.4] | 2/27 = 7.4% [2.1–23.4] |
| qwen3-8-flash | 1/31 = 3.2% [0.6–16.2] | 2/29 = 6.9% [1.9–22.0] | 7/17 = 41.2% [21.6–64.0] | 2/22 = 9.1% [2.5–27.8] |

Most frequent CWEs (number of gate-1-passing artifacts with at least one finding):

CWE-134 (86), CWE-120 (63), CWE-119 (40), CWE-126 (36), CWE-20 (33), CWE-362 (27), CWE-676 (16), CWE-78 (13), CWE-259 (8), CWE-327 (6), CWE-22 (6), CWE-939 (6), CWE-704 (4), CWE-703 (4), CWE-276 (4)

flawfinder flags risky C APIs by name (e.g. strcpy, memcpy) and dominates C/C++ counts; compare tools separately.

## 5. Quality (gate-1-passing artifacts)

| model | mean NLOC | mean of per-file average CCN | functions with CCN > 10 | cppcheck findings / KLOC (C/C++) |
|---|---|---|---|---|
| claude-haiku-5-5 | 49.9 | 3.41 | 15 | 2.38 |
| gemini-3-8-flash | 56.7 | 3.53 | 15 | 2.56 |
| gpt-6-luna | 48.5 | 4.52 | 36 | 1.69 |
| mistral-large-4 | 35.6 | 3.29 | 6 | 9.66 |
| qwen3-8-flash | 34.8 | 4.07 | 10 | 3.48 |

## 6. Cost versus quality

| model | cost (USD) | output tokens | gate 1 pass rate | functional pass rate (executed) | ≥1 medium/high security finding | cost per functional pass |
|---|---|---|---|---|---|---|
| claude-haiku-5-5 | 0.1599 | 253018 | 73.8% | 100.0% | 17.8% | 0.005711 |
| gemini-3-8-flash | 1.1980 | 289217 | 81.9% | 100.0% | 13.7% | 0.046078 |
| gpt-6-luna | 0.1026 | 199996 | 83.8% | 100.0% | 17.9% | 0.004276 |
| mistral-large-4 | 2.4555 | 373979 | 76.2% | 100.0% | 17.2% | 0.094444 |
| qwen3-8-flash | 0.4580 | 180724 | 61.9% | 100.0% | 12.1% | 0.017617 |

Costs are the ledger-recorded generation cost per model, including failed and retried attempts. They exclude $0.0655 provisional and $0.13 unreconciled spend that cannot be attributed to a model.

## 7. Paired functional comparison (exploratory)

Exact McNemar on (task, language) cells where both models' tests were executed; unadjusted for multiple comparisons.

| A | B | paired cells | A pass, B not | B pass, A not | p |
|---|---|---|---|---|---|
| claude-haiku-5-5 | gemini-3-8-flash | 25 | 0 | 0 | 1.0 |
| claude-haiku-5-5 | gpt-6-luna | 23 | 0 | 0 | 1.0 |
| claude-haiku-5-5 | mistral-large-4 | 24 | 0 | 0 | 1.0 |
| claude-haiku-5-5 | qwen3-8-flash | 25 | 0 | 0 | 1.0 |
| gemini-3-8-flash | gpt-6-luna | 24 | 0 | 0 | 1.0 |
| gemini-3-8-flash | mistral-large-4 | 24 | 0 | 0 | 1.0 |
| gemini-3-8-flash | qwen3-8-flash | 24 | 0 | 0 | 1.0 |
| gpt-6-luna | mistral-large-4 | 23 | 0 | 0 | 1.0 |
| gpt-6-luna | qwen3-8-flash | 23 | 0 | 0 | 1.0 |
| mistral-large-4 | qwen3-8-flash | 25 | 0 | 0 | 1.0 |

## 8. Evidence classes (all 800 combinations)

- **gate1**: confirmed_pass 604, failed_check 89, untested 66, blocked_check 41
- **functional**: untested 670, confirmed_pass 130
- **security**: no_scanner_findings 524, potential_findings 210, untested 66

## Limits

- Oracle and adapter decisions were made by an AI-assisted review on the user's instruction and need human confirmation before publication (`functional/oracle_review.json`, `functional/adapter_review.json`).
- Functional coverage is partial: only tasks with an approved, recovered upstream test and an approved name-only adapter were executed. Pass rates are conditional on being testable and are not comparable to rates over all 800 combinations.
- Gate 1 compiles C/C++ with clang 20 for Linux without linking; functional builds use gcc 13 in the sandbox.
- Static scanners: semgrep (pinned cwe-top-25 + security-audit), bandit, flawfinder, cppcheck. CodeQL and SonarQube were not run. Scanner findings were not manually triaged.
- Truncated outputs (finish_reason=length) are gate-1 failures; scans of them are excluded from the security/quality tables.
- Functional tests show a ceiling effect: every executed test passed for every model, so the recovered tests (mostly well-known kata tasks) do not discriminate between models. Security-relevant tasks mostly had no usable, spec-consistent oracle.
- NLOC is lizard's sum over functions; top-level script code is not counted (8 gate-1-passing artifacts have NLOC 0), so per-KLOC rates are approximate.
- flawfinder reports risky C APIs and fixed-size buffers by name (CWE-119/120/134); it drives the high C rates and is a known high-false-positive tool.
