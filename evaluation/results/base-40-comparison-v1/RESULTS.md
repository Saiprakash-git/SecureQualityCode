# Secondary analysis: our cohort vs the base study's published outputs (C, C++, Python)

Secondary and descriptive; the main result is the five-model experiment. Protocol: `evaluation/base_compare/BASE_COMPARISON_PROTOCOL.md` (approved; Java excluded). Same gate 1 code, toolchain, flags, scanner versions, rule hashes and configuration as the main benchmark (`scan_manifest.json` versions and config identical).

Base snapshot `1736c9c488b2`: 600 published files, 439 identical to the base test copies, 37 C/C++ files with a doubled-backslash escape. Published originals were copied byte-for-byte, never edited, and re-hashed after the run (0 changed).

Differences are ours minus base, in percentage points, with Newcombe hybrid-score 95% intervals (independent-sample approximation; the cohorts share tasks, see the paired check). Scanner hits are potential findings, not confirmed vulnerabilities.

## Primary

| language | measure | ours | base | diff (pp) [95% CI] |
|---|---|---|---|---|
| C | gate1_pass_planned | 117/200 = 58.5% [51.6, 65.1] | 142/200 = 71.0% [64.4, 76.8] | -12.5 [-21.6, -3.1] |
| C | gate1_pass_of_artifacts | 117/175 = 66.9% [59.6, 73.4] | 142/200 = 71.0% [64.4, 76.8] | -4.1 [-13.5, 5.2] |
| C | sec_medhigh_of_pass | 63/117 = 53.8% [44.8, 62.6] | 68/142 = 47.9% [39.8, 56.1] | 6.0 [-6.2, 17.9] |
| C | mean_nloc | 53.7009 | 21.169 | 32.5318 (no interval) |
| C | mean_avg_ccn | 5.4607 | 3.2422 | 2.2185 (no interval) |
| C | mean_ccd | 0.3025 | 0.2457 | 0.0568 (no interval) |
| C | pooled_ccd | 0.3061 | 0.2369 | 0.0692 (no interval) |
| C | cppcheck_per_kloc_c_cpp | 6.2072 | 27.9441 | -21.7369 (no interval) |
| C++ | gate1_pass_planned | 131/200 = 65.5% [58.7, 71.7] | 142/200 = 71.0% [64.4, 76.8] | -5.5 [-14.5, 3.6] |
| C++ | gate1_pass_of_artifacts | 131/183 = 71.6% [64.7, 77.6] | 142/200 = 71.0% [64.4, 76.8] | 0.6 [-8.5, 9.6] |
| C++ | sec_medhigh_of_pass | 11/131 = 8.4% [4.8, 14.4] | 15/142 = 10.6% [6.5, 16.7] | -2.2 [-9.3, 5.1] |
| C++ | mean_nloc | 48.4275 | 17.4085 | 31.019 (no interval) |
| C++ | mean_avg_ccn | 3.4535 | 2.6817 | 0.7718 (no interval) |
| C++ | mean_ccd | 0.277 | 0.2512 | 0.0257 (no interval) |
| C++ | pooled_ccd | 0.2659 | 0.2423 | 0.0236 (no interval) |
| C++ | cppcheck_per_kloc_c_cpp | 9.3001 | 22.2492 | -12.9491 (no interval) |
| Python | gate1_pass_planned | 186/200 = 93.0% [88.6, 95.8] | 198/200 = 99.0% [96.4, 99.7] | -6.0 [-10.5, -2.2] |
| Python | gate1_pass_of_artifacts | 186/187 = 99.5% [97.0, 99.9] | 198/200 = 99.0% [96.4, 99.7] | 0.5 [-2.1, 3.1] |
| Python | sec_medhigh_of_pass | 11/186 = 5.9% [3.3, 10.3] | 26/198 = 13.1% [9.1, 18.5] | -7.2 [-13.2, -1.3] |
| Python | mean_nloc | 36.0591 | 7.0253 | 29.0339 (no interval) |
| Python | mean_avg_ccn | 3.0774 | 2.15 | 0.9274 (no interval) |
| Python | mean_ccd | 0.414 | 0.4233 | -0.0093 (no interval) |
| Python | pooled_ccd | 0.3082 | 0.348 | -0.0398 (no interval) |
| All three | gate1_pass_planned | 434/600 = 72.3% [68.6, 75.8] | 482/600 = 80.3% [77.0, 83.3] | -8.0 [-12.8, -3.2] |
| All three | gate1_pass_of_artifacts | 434/545 = 79.6% [76.0, 82.8] | 482/600 = 80.3% [77.0, 83.3] | -0.7 [-5.4, 3.9] |
| All three | sec_medhigh_of_pass | 85/434 = 19.6% [16.1, 23.6] | 109/482 = 22.6% [19.1, 26.6] | -3.0 [-8.3, 2.3] |
| All three | mean_nloc | 44.5484 | 14.251 | 30.2973 (no interval) |
| All three | mean_avg_ccn | 3.845 | 2.6448 | 1.2001 (no interval) |
| All three | mean_ccd | 0.3417 | 0.3161 | 0.0256 (no interval) |
| All three | pooled_ccd | 0.2936 | 0.2613 | 0.0323 (no interval) |
| All three | cppcheck_per_kloc_c_cpp | 7.7611 | 25.3742 | -17.6131 (no interval) |

Paired check per (task, language) cell, number of models passing gate 1 (0-5), Wilcoxon signed-rank (SciPy, zero_method='wilcox'):

| language | cells | ours higher | equal | base higher | mean diff | W | p |
|---|---|---|---|---|---|---|---|
| C | 40 | 6 | 17 | 17 | -0.625 | 51.0 | 6.76e-03 |
| C++ | 40 | 6 | 22 | 12 | -0.275 | 55.5 | 1.80e-01 |
| Python | 40 | 1 | 29 | 10 | -0.300 | 4.5 | 8.00e-03 |
| All three | 120 | 13 | 68 | 39 | -0.400 | 293.5 | 2.13e-04 |

## S1: base files identical to the test copies only

| language | measure | ours | base | diff (pp) [95% CI] |
|---|---|---|---|---|
| C | gate1_pass_planned | 117/200 = 58.5% [51.6, 65.1] | 110/139 = 79.1% [71.6, 85.1] | -20.6 [-29.8, -10.6] |
| C | gate1_pass_of_artifacts | 117/175 = 66.9% [59.6, 73.4] | 110/139 = 79.1% [71.6, 85.1] | -12.3 [-21.7, -2.3] |
| C | sec_medhigh_of_pass | 63/117 = 53.8% [44.8, 62.6] | 59/110 = 53.6% [44.4, 62.7] | 0.2 [-12.6, 13.0] |
| C | mean_nloc | 53.7009 | 20.9545 | 32.7463 (no interval) |
| C | mean_avg_ccn | 5.4607 | 3.2755 | 2.1852 (no interval) |
| C | mean_ccd | 0.3025 | 0.2395 | 0.063 (no interval) |
| C | pooled_ccd | 0.3061 | 0.233 | 0.0731 (no interval) |
| C | cppcheck_per_kloc_c_cpp | 6.2072 | 23.8612 | -17.6539 (no interval) |
| C++ | gate1_pass_planned | 131/200 = 65.5% [58.7, 71.7] | 88/110 = 80.0% [71.6, 86.4] | -14.5 [-23.9, -4.0] |
| C++ | gate1_pass_of_artifacts | 131/183 = 71.6% [64.7, 77.6] | 88/110 = 80.0% [71.6, 86.4] | -8.4 [-17.8, 2.0] |
| C++ | sec_medhigh_of_pass | 11/131 = 8.4% [4.8, 14.4] | 8/88 = 9.1% [4.7, 16.9] | -0.7 [-9.3, 6.8] |
| C++ | mean_nloc | 48.4275 | 16.1136 | 32.3138 (no interval) |
| C++ | mean_avg_ccn | 3.4535 | 2.874 | 0.5796 (no interval) |
| C++ | mean_ccd | 0.277 | 0.2603 | 0.0167 (no interval) |
| C++ | pooled_ccd | 0.2659 | 0.2489 | 0.017 (no interval) |
| C++ | cppcheck_per_kloc_c_cpp | 9.3001 | 23.2722 | -13.9721 (no interval) |
| Python | gate1_pass_planned | 186/200 = 93.0% [88.6, 95.8] | 190/190 = 100.0% [98.0, 100.0] | -7.0 [-11.4, -3.6] |
| Python | gate1_pass_of_artifacts | 186/187 = 99.5% [97.0, 99.9] | 190/190 = 100.0% [98.0, 100.0] | -0.5 [-3.0, 1.5] |
| Python | sec_medhigh_of_pass | 11/186 = 5.9% [3.3, 10.3] | 26/190 = 13.7% [9.5, 19.3] | -7.8 [-13.9, -1.7] |
| Python | mean_nloc | 36.0591 | 6.8474 | 29.2118 (no interval) |
| Python | mean_avg_ccn | 3.0774 | 2.0498 | 1.0276 (no interval) |
| Python | mean_ccd | 0.414 | 0.4137 | 0.0002 (no interval) |
| Python | pooled_ccd | 0.3082 | 0.3328 | -0.0246 (no interval) |
| All three | gate1_pass_planned | 434/600 = 72.3% [68.6, 75.8] | 388/439 = 88.4% [85.0, 91.1] | -16.0 [-20.6, -11.3] |
| All three | gate1_pass_of_artifacts | 434/545 = 79.6% [76.0, 82.8] | 388/439 = 88.4% [85.0, 91.1] | -8.7 [-13.2, -4.1] |
| All three | sec_medhigh_of_pass | 85/434 = 19.6% [16.1, 23.6] | 93/388 = 24.0% [20.0, 28.5] | -4.4 [-10.1, 1.3] |
| All three | mean_nloc | 44.5484 | 12.9485 | 31.5999 (no interval) |
| All three | mean_avg_ccn | 3.845 | 2.6027 | 1.2423 (no interval) |
| All three | mean_ccd | 0.3417 | 0.3264 | 0.0153 (no interval) |
| All three | pooled_ccd | 0.2936 | 0.2633 | 0.0303 (no interval) |
| All three | cppcheck_per_kloc_c_cpp | 7.7611 | 23.6369 | -15.8757 (no interval) |

## S2: base C/C++ files with doubled-backslash escapes excluded

| language | measure | ours | base | diff (pp) [95% CI] |
|---|---|---|---|---|
| C | gate1_pass_planned | 117/200 = 58.5% [51.6, 65.1] | 119/166 = 71.7% [64.4, 78.0] | -13.2 [-22.6, -3.4] |
| C | gate1_pass_of_artifacts | 117/175 = 66.9% [59.6, 73.4] | 119/166 = 71.7% [64.4, 78.0] | -4.8 [-14.5, 5.0] |
| C | sec_medhigh_of_pass | 63/117 = 53.8% [44.8, 62.6] | 52/119 = 43.7% [35.1, 52.7] | 10.1 [-2.6, 22.4] |
| C | mean_nloc | 53.7009 | 19.3445 | 34.3563 (no interval) |
| C | mean_avg_ccn | 5.4607 | 3.2189 | 2.2418 (no interval) |
| C | mean_ccd | 0.3025 | 0.2468 | 0.0557 (no interval) |
| C | pooled_ccd | 0.3061 | 0.2389 | 0.0671 (no interval) |
| C | cppcheck_per_kloc_c_cpp | 6.2072 | 26.9331 | -20.7259 (no interval) |
| C++ | gate1_pass_planned | 131/200 = 65.5% [58.7, 71.7] | 141/197 = 71.6% [64.9, 77.4] | -6.1 [-15.1, 3.1] |
| C++ | gate1_pass_of_artifacts | 131/183 = 71.6% [64.7, 77.6] | 141/197 = 71.6% [64.9, 77.4] | 0.0 [-9.0, 9.0] |
| C++ | sec_medhigh_of_pass | 11/131 = 8.4% [4.8, 14.4] | 15/141 = 10.6% [6.6, 16.8] | -2.2 [-9.4, 5.0] |
| C++ | mean_nloc | 48.4275 | 17.2624 | 31.1651 (no interval) |
| C++ | mean_avg_ccn | 3.4535 | 2.6793 | 0.7742 (no interval) |
| C++ | mean_ccd | 0.277 | 0.2513 | 0.0256 (no interval) |
| C++ | pooled_ccd | 0.2659 | 0.2424 | 0.0235 (no interval) |
| C++ | cppcheck_per_kloc_c_cpp | 9.3001 | 21.7749 | -12.4747 (no interval) |
| Python | gate1_pass_planned | 186/200 = 93.0% [88.6, 95.8] | 198/200 = 99.0% [96.4, 99.7] | -6.0 [-10.5, -2.2] |
| Python | gate1_pass_of_artifacts | 186/187 = 99.5% [97.0, 99.9] | 198/200 = 99.0% [96.4, 99.7] | 0.5 [-2.1, 3.1] |
| Python | sec_medhigh_of_pass | 11/186 = 5.9% [3.3, 10.3] | 26/198 = 13.1% [9.1, 18.5] | -7.2 [-13.2, -1.3] |
| Python | mean_nloc | 36.0591 | 7.0253 | 29.0339 (no interval) |
| Python | mean_avg_ccn | 3.0774 | 2.15 | 0.9274 (no interval) |
| Python | mean_ccd | 0.414 | 0.4233 | -0.0093 (no interval) |
| Python | pooled_ccd | 0.3082 | 0.348 | -0.0398 (no interval) |
| All three | gate1_pass_planned | 434/600 = 72.3% [68.6, 75.8] | 458/563 = 81.3% [77.9, 84.4] | -9.0 [-13.8, -4.2] |
| All three | gate1_pass_of_artifacts | 434/545 = 79.6% [76.0, 82.8] | 458/563 = 81.3% [77.9, 84.4] | -1.7 [-6.4, 2.9] |
| All three | sec_medhigh_of_pass | 85/434 = 19.6% [16.1, 23.6] | 93/458 = 20.3% [16.9, 24.2] | -0.7 [-6.0, 4.5] |
| All three | mean_nloc | 44.5484 | 13.3777 | 31.1707 (no interval) |
| All three | mean_avg_ccn | 3.845 | 2.6042 | 1.2408 (no interval) |
| All three | mean_ccd | 0.3417 | 0.3205 | 0.0212 (no interval) |
| All three | pooled_ccd | 0.2936 | 0.2651 | 0.0286 (no interval) |
| All three | cppcheck_per_kloc_c_cpp | 7.7611 | 24.2821 | -16.5209 (no interval) |

## S3: blocked_dependency removed from the gate-1 denominator (both cohorts)

| language | measure | ours | base | diff (pp) [95% CI] |
|---|---|---|---|---|
| C | gate1_pass_planned | 117/180 = 65.0% [57.8, 71.6] | 142/157 = 90.4% [84.8, 94.1] | -25.4 [-33.5, -16.8] |
| C | gate1_pass_of_artifacts | 117/155 = 75.5% [68.1, 81.6] | 142/157 = 90.4% [84.8, 94.1] | -15.0 [-23.2, -6.7] |
| C | sec_medhigh_of_pass | 63/117 = 53.8% [44.8, 62.6] | 68/142 = 47.9% [39.8, 56.1] | 6.0 [-6.2, 17.9] |
| C | mean_nloc | 53.7009 | 21.169 | 32.5318 (no interval) |
| C | mean_avg_ccn | 5.4607 | 3.2422 | 2.2185 (no interval) |
| C | mean_ccd | 0.3025 | 0.2457 | 0.0568 (no interval) |
| C | pooled_ccd | 0.3061 | 0.2369 | 0.0692 (no interval) |
| C | cppcheck_per_kloc_c_cpp | 6.2072 | 27.9441 | -21.7369 (no interval) |
| C++ | gate1_pass_planned | 131/179 = 73.2% [66.3, 79.1] | 142/150 = 94.7% [89.8, 97.3] | -21.5 [-28.9, -13.8] |
| C++ | gate1_pass_of_artifacts | 131/162 = 80.9% [74.1, 86.2] | 142/150 = 94.7% [89.8, 97.3] | -13.8 [-21.0, -6.6] |
| C++ | sec_medhigh_of_pass | 11/131 = 8.4% [4.8, 14.4] | 15/142 = 10.6% [6.5, 16.7] | -2.2 [-9.3, 5.1] |
| C++ | mean_nloc | 48.4275 | 17.4085 | 31.019 (no interval) |
| C++ | mean_avg_ccn | 3.4535 | 2.6817 | 0.7718 (no interval) |
| C++ | mean_ccd | 0.277 | 0.2512 | 0.0257 (no interval) |
| C++ | pooled_ccd | 0.2659 | 0.2423 | 0.0236 (no interval) |
| C++ | cppcheck_per_kloc_c_cpp | 9.3001 | 22.2492 | -12.9491 (no interval) |
| Python | gate1_pass_planned | 186/200 = 93.0% [88.6, 95.8] | 198/200 = 99.0% [96.4, 99.7] | -6.0 [-10.5, -2.2] |
| Python | gate1_pass_of_artifacts | 186/187 = 99.5% [97.0, 99.9] | 198/200 = 99.0% [96.4, 99.7] | 0.5 [-2.1, 3.1] |
| Python | sec_medhigh_of_pass | 11/186 = 5.9% [3.3, 10.3] | 26/198 = 13.1% [9.1, 18.5] | -7.2 [-13.2, -1.3] |
| Python | mean_nloc | 36.0591 | 7.0253 | 29.0339 (no interval) |
| Python | mean_avg_ccn | 3.0774 | 2.15 | 0.9274 (no interval) |
| Python | mean_ccd | 0.414 | 0.4233 | -0.0093 (no interval) |
| Python | pooled_ccd | 0.3082 | 0.348 | -0.0398 (no interval) |
| All three | gate1_pass_planned | 434/559 = 77.6% [74.0, 80.9] | 482/507 = 95.1% [92.8, 96.6] | -17.4 [-21.4, -13.5] |
| All three | gate1_pass_of_artifacts | 434/504 = 86.1% [82.8, 88.9] | 482/507 = 95.1% [92.8, 96.6] | -9.0 [-12.6, -5.4] |
| All three | sec_medhigh_of_pass | 85/434 = 19.6% [16.1, 23.6] | 109/482 = 22.6% [19.1, 26.6] | -3.0 [-8.3, 2.3] |
| All three | mean_nloc | 44.5484 | 14.251 | 30.2973 (no interval) |
| All three | mean_avg_ccn | 3.845 | 2.6448 | 1.2001 (no interval) |
| All three | mean_ccd | 0.3417 | 0.3161 | 0.0256 (no interval) |
| All three | pooled_ccd | 0.2936 | 0.2613 | 0.0323 (no interval) |
| All three | cppcheck_per_kloc_c_cpp | 7.7611 | 25.3742 | -17.6131 (no interval) |

## S4 (post-hoc, not pre-declared): our files re-graded without the ledger-only truncation check

| language | measure | ours | base | diff (pp) [95% CI] |
|---|---|---|---|---|
| C | gate1_pass_planned | 118/200 = 59.0% [52.1, 65.6] | 142/200 = 71.0% [64.4, 76.8] | -12.0 [-21.1, -2.7] |
| C | gate1_pass_of_artifacts | 118/175 = 67.4% [60.2, 73.9] | 142/200 = 71.0% [64.4, 76.8] | -3.6 [-12.9, 5.7] |
| C | sec_medhigh_of_pass | 64/118 = 54.2% [45.3, 63.0] | 68/142 = 47.9% [39.8, 56.1] | 6.3 [-5.8, 18.2] |
| C | mean_nloc | 54.2034 | 21.169 | 33.0344 (no interval) |
| C | mean_avg_ccn | 5.4423 | 3.2422 | 2.2001 (no interval) |
| C | mean_ccd | 0.3017 | 0.2457 | 0.056 (no interval) |
| C | pooled_ccd | 0.3043 | 0.2369 | 0.0674 (no interval) |
| C | cppcheck_per_kloc_c_cpp | 6.0976 | 27.9441 | -21.8466 (no interval) |
| C++ | gate1_pass_planned | 131/200 = 65.5% [58.7, 71.7] | 142/200 = 71.0% [64.4, 76.8] | -5.5 [-14.5, 3.6] |
| C++ | gate1_pass_of_artifacts | 131/183 = 71.6% [64.7, 77.6] | 142/200 = 71.0% [64.4, 76.8] | 0.6 [-8.5, 9.6] |
| C++ | sec_medhigh_of_pass | 11/131 = 8.4% [4.8, 14.4] | 15/142 = 10.6% [6.5, 16.7] | -2.2 [-9.3, 5.1] |
| C++ | mean_nloc | 48.4275 | 17.4085 | 31.019 (no interval) |
| C++ | mean_avg_ccn | 3.4535 | 2.6817 | 0.7718 (no interval) |
| C++ | mean_ccd | 0.277 | 0.2512 | 0.0257 (no interval) |
| C++ | pooled_ccd | 0.2659 | 0.2423 | 0.0236 (no interval) |
| C++ | cppcheck_per_kloc_c_cpp | 9.3001 | 22.2492 | -12.9491 (no interval) |
| Python | gate1_pass_planned | 187/200 = 93.5% [89.2, 96.2] | 198/200 = 99.0% [96.4, 99.7] | -5.5 [-9.9, -1.8] |
| Python | gate1_pass_of_artifacts | 187/187 = 100.0% [98.0, 100.0] | 198/200 = 99.0% [96.4, 99.7] | 1.0 [-1.1, 3.6] |
| Python | sec_medhigh_of_pass | 11/187 = 5.9% [3.3, 10.2] | 26/198 = 13.1% [9.1, 18.5] | -7.2 [-13.2, -1.3] |
| Python | mean_nloc | 35.8663 | 7.0253 | 28.8411 (no interval) |
| Python | mean_avg_ccn | 3.0774 | 2.15 | 0.9274 (no interval) |
| Python | mean_ccd | 0.414 | 0.4233 | -0.0093 (no interval) |
| Python | pooled_ccd | 0.3082 | 0.348 | -0.0398 (no interval) |
| All three | gate1_pass_planned | 436/600 = 72.7% [69.0, 76.1] | 482/600 = 80.3% [77.0, 83.3] | -7.7 [-12.4, -2.9] |
| All three | gate1_pass_of_artifacts | 436/545 = 80.0% [76.4, 83.1] | 482/600 = 80.3% [77.0, 83.3] | -0.3 [-5.0, 4.3] |
| All three | sec_medhigh_of_pass | 86/436 = 19.7% [16.3, 23.7] | 109/482 = 22.6% [19.1, 26.6] | -2.9 [-8.1, 2.4] |
| All three | mean_nloc | 44.6032 | 14.251 | 30.3522 (no interval) |
| All three | mean_avg_ccn | 3.8437 | 2.6448 | 1.1988 (no interval) |
| All three | mean_ccd | 0.3414 | 0.3161 | 0.0253 (no interval) |
| All three | pooled_ccd | 0.2931 | 0.2613 | 0.0318 (no interval) |
| All three | cppcheck_per_kloc_c_cpp | 7.6923 | 25.3742 | -17.6819 (no interval) |

Paired check per (task, language) cell, number of models passing gate 1 (0-5), Wilcoxon signed-rank (SciPy, zero_method='wilcox'):

| language | cells | ours higher | equal | base higher | mean diff | W | p |
|---|---|---|---|---|---|---|---|
| C | 40 | 6 | 17 | 17 | -0.600 | 51.5 | 7.05e-03 |
| C++ | 40 | 6 | 22 | 12 | -0.275 | 55.5 | 1.80e-01 |
| Python | 40 | 1 | 29 | 10 | -0.275 | 5.0 | 7.97e-03 |
| All three | 120 | 13 | 68 | 39 | -0.383 | 302.0 | 2.81e-04 |

## Gate-1 outcome counts (primary)

| language | ours | base |
|---|---|---|
| C | {'fail_integrity': 37, 'no_artifact': 25, 'pass': 117, 'fail_compile': 1, 'blocked_dependency': 20} | {'pass': 142, 'blocked_dependency': 43, 'fail_compile': 15} |
| C++ | {'pass': 131, 'no_artifact': 17, 'fail_compile': 1, 'fail_integrity': 30, 'blocked_dependency': 21} | {'pass': 142, 'blocked_dependency': 50, 'fail_compile': 8} |
| Python | {'pass': 186, 'no_artifact': 13, 'fail_integrity': 1} | {'pass': 198, 'fail_syntax': 2} |
| All three | {'pass': 434, 'no_artifact': 55, 'fail_integrity': 68, 'fail_compile': 2, 'blocked_dependency': 41} | {'pass': 482, 'blocked_dependency': 93, 'fail_compile': 23, 'fail_syntax': 2} |

## Confounders (differences are attributed to cohorts, never to models)

- Model cohort: five 2024-era models (base) vs five current cost-efficient models (ours).
- Prompt: the base study's prompt wording was not reproduced.
- Temperature: 0.9 in the base study's generation scripts vs 0.2.
- Output-token limits and retry policy differ; our max_tokens 4096 produced truncations that fail gate 1.
- Extraction: the base pipeline leaves doubled-backslash escapes (likely artifact); ours validates raw source.
- Generation dates are about two years apart.
- Unidentifiable manual fixes: the published files may contain some of the base study's manual fixes; no fix list exists.
- File size: base files are much shorter (mean function NLOC), which confounds per-file and per-KLOC rates.

## Limitations

- Java excluded (no published Java outputs).
- Truncation is detectable only for our files (ledger finish_reason); S4 shows the effect is small (2 files).
- S1 is a selected subset: files untouched by post-processing may be those that needed no fixes.
- The 4 language cells of a task, and both cohorts, share tasks; intervals treat cohorts as independent samples.
- Functional tests and the base study's CodeQL/SonarQube numbers are not compared (different tests, tools and inputs).
