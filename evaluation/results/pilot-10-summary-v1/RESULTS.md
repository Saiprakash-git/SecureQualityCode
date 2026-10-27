# Pilot-10: supplementary disjoint-task check

Tasks 24, 37, 38, 39, 41, 42, 43, 46, 51, 104 (all from the security strata A-D); same five models, settings and prompt as benchmark-40. The pilot runs were used to validate the generation runner, so these results are reported separately and never pooled with the 40-task study. Same gate 1 code and scanner configuration as the main evaluation. Functional tests were not run (no oracle review for these tasks). Scanner hits are potential findings, not confirmed vulnerabilities.

Snapshot `d369e321bc56`: 200 planned, 184 artifacts, status {'failed': 15, 'generated': 155, 'generated_truncated': 29, 'incomplete': 1} (the one `incomplete` combination is a Qwen3.8 Flash request that ended in IncompleteRead; it counts as not passing). Cross-check of per-artifact scanner values against raw output: 0 mismatches.

Gate 1 outcomes: {'pass': 142, 'blocked_dependency': 12, 'fail_integrity': 30, 'no_artifact': 16}

| group | gate 1 pass / planned | gate 1 pass / artifact | >=1 medium/high potential finding (of passes) | mean NLOC | mean CCN | mean CCD | Cppcheck/KLOC (C/C++) |
|---|---|---|---|---|---|---|---|
| **All** | 142/200 = 71.0% [64.4, 76.8] | 142/184 = 77.2% [70.6, 82.6] | 36/142 = 25.4% [18.9, 33.1] | 52.2 | 3.61 | 0.323 | 8.49 |
| claude-haiku-5-5 | 27/40 = 67.5% [52.0, 79.9] | 27/37 = 73.0% [57.0, 84.6] | 7/27 = 25.9% [13.2, 44.7] | 57.9 | 3.40 | 0.311 | 4.15 |
| gpt-6-luna | 30/40 = 75.0% [59.8, 85.8] | 30/40 = 75.0% [59.8, 85.8] | 7/30 = 23.3% [11.8, 40.9] | 49.3 | 4.02 | 0.357 | 5.61 |
| gemini-3-8-flash | 30/40 = 75.0% [59.8, 85.8] | 30/40 = 75.0% [59.8, 85.8] | 7/30 = 23.3% [11.8, 40.9] | 66.3 | 3.48 | 0.302 | 11.02 |
| mistral-large-4 | 29/40 = 72.5% [57.2, 83.9] | 29/35 = 82.9% [67.3, 91.9] | 8/29 = 27.6% [14.7, 45.7] | 41.7 | 3.09 | 0.320 | 13.14 |
| qwen3-8-flash | 26/40 = 65.0% [49.5, 77.9] | 26/32 = 81.2% [64.7, 91.1] | 7/26 = 26.9% [13.7, 46.1] | 44.8 | 4.12 | 0.322 | 6.00 |
| python | 46/50 = 92.0% [81.2, 96.8] | 46/48 = 95.8% [86.0, 98.8] | 3/46 = 6.5% [2.2, 17.5] | 50.1 | 2.74 | 0.366 | n/a |
| java | 39/50 = 78.0% [64.8, 87.2] | 39/48 = 81.2% [68.1, 89.8] | 1/39 = 2.6% [0.5, 13.2] | 55.5 | 3.32 | 0.289 | n/a |
| c | 28/50 = 56.0% [42.3, 68.8] | 28/44 = 63.6% [48.9, 76.2] | 21/28 = 75.0% [56.6, 87.3] | 55.4 | 5.50 | 0.327 | 9.67 |
| cpp | 29/50 = 58.0% [44.2, 70.6] | 29/44 = 65.9% [51.1, 78.1] | 11/29 = 37.9% [22.7, 56.0] | 48.0 | 3.54 | 0.298 | 7.18 |

Cell sizes are small (40 combinations per model, 50 per language); intervals are wide. Not comparable one-to-one with benchmark-40: the pilot contains only security-strata tasks.
