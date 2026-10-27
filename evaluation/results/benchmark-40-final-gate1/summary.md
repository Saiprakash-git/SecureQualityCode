# Gate 1 summary: benchmark-40-final-gate1

Evaluator m1.0. Inventory snapshot of 800 planned combinations.

## Inventory

| status | count |
|---|---|
| failed | 66 |
| generated | 654 |
| generated_truncated | 80 |

## Gate 1 funnel (all rows)

| gate 1 status | count |
|---|---|
| pass | 604 |
| fail_integrity | 85 |
| fail_syntax | 2 |
| fail_compile | 2 |
| blocked_dependency | 41 |
| toolchain_error | 0 |
| no_artifact | 66 |

- Pass over generation-final combinations: 604/800 = 75.5% [72.4, 78.3]
- Pass over evaluable artifacts (excludes dependency-blocked and toolchain errors): 604/693 = 87.2% [84.5, 89.4]
- Truncated outputs that still compiled (counted as fail_integrity): 2

## By model and language (pass over generation-final, Wilson 95% CI)

| model | python | java | c | cpp |
|---|---|---|---|---|
| claude-haiku-5-5 | 39/40 = 97.5% [87.1, 99.6] | 31/40 = 77.5% [62.5, 87.7] | 25/40 = 62.5% [47.0, 75.8] | 23/40 = 57.5% [42.2, 71.5] |
| gemini-3-8-flash | 39/40 = 97.5% [87.1, 99.6] | 38/40 = 95.0% [83.5, 98.6] | 25/40 = 62.5% [47.0, 75.8] | 29/40 = 72.5% [57.2, 83.9] |
| gpt-6-luna | 40/40 = 100.0% [91.2, 100.0] | 37/40 = 92.5% [80.1, 97.4] | 27/40 = 67.5% [52.0, 79.9] | 30/40 = 75.0% [59.8, 85.8] |
| mistral-large-4 | 37/40 = 92.5% [80.1, 97.4] | 35/40 = 87.5% [73.9, 94.5] | 23/40 = 57.5% [42.2, 71.5] | 27/40 = 67.5% [52.0, 79.9] |
| qwen3-8-flash | 31/40 = 77.5% [62.5, 87.7] | 29/40 = 72.5% [57.2, 83.9] | 17/40 = 42.5% [28.5, 57.8] | 22/40 = 55.0% [39.8, 69.3] |

## Paired comparison (160 of 160 task-language cells complete for every model)

Exact McNemar, unadjusted for multiple comparisons; exploratory.

| model A | model B | A pass, B not | B pass, A not | p |
|---|---|---|---|---|
| claude-haiku-5-5 | gpt-6-luna | 2 | 18 | 0.000402 |
| claude-haiku-5-5 | gemini-3-8-flash | 3 | 16 | 0.004425 |
| claude-haiku-5-5 | mistral-large-4 | 11 | 15 | 0.557197 |
| claude-haiku-5-5 | qwen3-8-flash | 23 | 4 | 0.000311 |
| gpt-6-luna | gemini-3-8-flash | 7 | 4 | 0.548828 |
| gpt-6-luna | mistral-large-4 | 16 | 4 | 0.011818 |
| gpt-6-luna | qwen3-8-flash | 36 | 1 | 0.0 |
| gemini-3-8-flash | mistral-large-4 | 13 | 4 | 0.049042 |
| gemini-3-8-flash | qwen3-8-flash | 33 | 1 | 0.0 |
| mistral-large-4 | qwen3-8-flash | 29 | 6 | 0.000117 |

## Dependencies and integrity

- Integrity failures: {'markdown_fence_residue': 10, 'truncated_finish_length': 80}
- Missing third-party headers/packages (C, C++, Java): {'curl/curl.h': 8, 'ldap.h': 7, 'libpq-fe.h': 10, 'openssl/crypto.h': 2, 'openssl/err.h': 2, 'openssl/evp.h': 11, 'openssl/rsa.h': 3, 'openssl/ssl.h': 6, 'sqlite3.h': 8, 'windows.h': 3}
- Python third-party imports (not resolved at this gate): {'Crypto': 1, 'cryptography': 9, 'ldap3': 4, 'psycopg2': 5}

## Limits of this gate

- Passing gate 1 means the file is complete and compiles; it is not evidence of functional correctness.
- Python is checked for syntax only (ast + compile); imports are not resolved.
- C/C++ are compiled as single translation units for x86_64-linux-gnu (gnu17/gnu++17), without linking.
- Java is compiled with javac and no third-party classpath; missing packages are reported as blocked_dependency.
- Generated code was not executed.
