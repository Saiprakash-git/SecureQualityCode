# Gate 1 summary: pilot-10-gate1-v1

Evaluator m1.0. Inventory snapshot of 200 planned combinations.

## Inventory

| status | count |
|---|---|
| failed | 15 |
| generated | 155 |
| generated_truncated | 29 |
| incomplete | 1 |

## Gate 1 funnel (all rows)

| gate 1 status | count |
|---|---|
| pass | 142 |
| fail_integrity | 30 |
| fail_syntax | 0 |
| fail_compile | 0 |
| blocked_dependency | 12 |
| toolchain_error | 0 |
| no_artifact | 16 |

- Pass over generation-final combinations: 142/199 = 71.4% [64.7, 77.2]
- Pass over evaluable artifacts (excludes dependency-blocked and toolchain errors): 142/172 = 82.6% [76.2, 87.5]
- Truncated outputs that still compiled (counted as fail_integrity): 2

## By model and language (pass over generation-final, Wilson 95% CI)

| model | python | java | c | cpp |
|---|---|---|---|---|
| claude-haiku-5-5 | 9/10 = 90.0% [59.6, 98.2] | 8/10 = 80.0% [49.0, 94.3] | 5/10 = 50.0% [23.7, 76.3] | 5/10 = 50.0% [23.7, 76.3] |
| gemini-3-8-flash | 9/10 = 90.0% [59.6, 98.2] | 9/10 = 90.0% [59.6, 98.2] | 6/10 = 60.0% [31.3, 83.2] | 6/10 = 60.0% [31.3, 83.2] |
| gpt-6-luna | 10/10 = 100.0% [72.2, 100.0] | 9/10 = 90.0% [59.6, 98.2] | 5/10 = 50.0% [23.7, 76.3] | 6/10 = 60.0% [31.3, 83.2] |
| mistral-large-4 | 9/10 = 90.0% [59.6, 98.2] | 7/10 = 70.0% [39.7, 89.2] | 6/10 = 60.0% [31.3, 83.2] | 7/10 = 70.0% [39.7, 89.2] |
| qwen3-8-flash | 9/10 = 90.0% [59.6, 98.2] | 6/10 = 60.0% [31.3, 83.2] | 6/10 = 60.0% [31.3, 83.2] | 5/9 = 55.6% [26.7, 81.1] |

## Paired comparison (39 of 40 task-language cells complete for every model)

Exact McNemar, unadjusted for multiple comparisons; exploratory.

| model A | model B | A pass, B not | B pass, A not | p |
|---|---|---|---|---|
| claude-haiku-5-5 | gpt-6-luna | 0 | 3 | 0.25 |
| claude-haiku-5-5 | gemini-3-8-flash | 0 | 3 | 0.25 |
| claude-haiku-5-5 | mistral-large-4 | 2 | 4 | 0.6875 |
| claude-haiku-5-5 | qwen3-8-flash | 2 | 1 | 1.0 |
| gpt-6-luna | gemini-3-8-flash | 1 | 1 | 1.0 |
| gpt-6-luna | mistral-large-4 | 3 | 2 | 1.0 |
| gpt-6-luna | qwen3-8-flash | 5 | 1 | 0.21875 |
| gemini-3-8-flash | mistral-large-4 | 3 | 2 | 1.0 |
| gemini-3-8-flash | qwen3-8-flash | 5 | 1 | 0.21875 |
| mistral-large-4 | qwen3-8-flash | 3 | 0 | 0.25 |

## Dependencies and integrity

- Integrity failures: {'markdown_fence_residue': 3, 'truncated_finish_length': 29}
- Missing third-party headers/packages (C, C++, Java): {'jakarta.servlet': 1, 'jakarta.servlet.annotation': 1, 'jakarta.servlet.http': 1, 'mysql/mysql.h': 1, 'openssl/crypto.h': 2, 'sqlite3.h': 11}
- Python third-party imports (not resolved at this gate): {'flask': 1}

## Limits of this gate

- Passing gate 1 means the file is complete and compiles; it is not evidence of functional correctness.
- Python is checked for syntax only (ast + compile); imports are not resolved.
- C/C++ are compiled as single translation units for x86_64-linux-gnu (gnu17/gnu++17), without linking.
- Java is compiled with javac and no third-party classpath; missing packages are reported as blocked_dependency.
- Generated code was not executed.
