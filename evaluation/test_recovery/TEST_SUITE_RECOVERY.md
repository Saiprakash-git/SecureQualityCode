# Recovering the original functional tests (investigation, 2026-10-26)

No test was run. No test result exists yet.

## Source

The dataset README ("Tests GitHub Repositories") points to four public repositories. Snapshot used:

| language | repository | commit (master) |
|---|---|---|
| Python | mohsystem/llm-generated-code-python | 8f438f770383783fc9fc62b4068f3f1b70e58e6b (2025-11-22) |
| Java | mohsystem/llm-generated-code-java | 70dbb6b13099714402cfc0083225faeb59c11067 (2025-11-26) |
| C | mohsystem/llm_generated_code_c | 8e3568277c0f6d68e18df6d530ee4367aa9b7c86 (2025-11-29) |
| C++ | mohsystem/llm-generated-code-c-cpp | 0435da599ecae1596822e2773994ffb84f26dec5 (2025-11-30) |

None of the four repositories declares a license (the GitHub license API returns 404). Ask the authors before redistributing their tests. Citing them and using them for evaluation is less of a concern.

## How the tests are built

- Tests are written for each of the five original models (claude-3.5-sonnet, gpt-4o, gemini-1.5-pro, codestral, llama-3). Examples: `tests/<model>/Task18_Test.c` and `src/test/java/<model>/task18/Task18_TestCase.java`.
- Each test calls that model's own function names and file paths. Examples: `#include "../../source/output/claude/c/Task18_...c"`, `convertBinaryArrayToInt` versus `binaryArrayToInt`, and Python imports that are commented out per model.
- The inputs and expected outputs are mostly the same across models. Only the binding to each model's code differs. Measured overlap of string and number literals between model pairs for the same task (median, with 25th percentile):

  | language | median | 25th percentile |
  |---|---|---|
  | Python | 1.00 | 0.92 |
  | C | 0.91 | 0.51 |
  | C++ | 1.00 | 0.88 |
  | Java | 0.68 | 0.17 |

  Java tests differ more between models.
- C/C++ tests `#include` the source file under test, so a generated `main()` clashes with the test's own `main`. The upstream build handles this with CMake. We would need a rename such as `-Dmain=...`, recorded as a harness change.
- Some Python "test" folders hold only a copy of the generated code, with no assertions (for example Task 56). Some files are empty: Java 9, C++ 6, C 3.

## Coverage for the 40 benchmark tasks

A task counts if at least one file contains checks: `assert`, `self.assert*`, `@Test`, or PASS/FAIL comparisons. The data is in `test_usability_40_v2.json`. File lists per task are in `test_coverage_40.json`.

| language | tasks with checks |
|---|---|
| Python | 21/40 |
| Java | 40/40 (regex finds `@Test`; whether the test bodies are real is not checked yet) |
| C | 25/40 |
| C++ | 24/40 |
| all four languages | 13/40: 3, 9, 12, 14, 17, 18, 21, 23, 25, 31, 44, 73, 82 |

These counts are upper bounds from a regex scan. A person still needs to check each one.

## What this means for gate 2 (functional)

1. The tests can't be run as they are against our new models' outputs, because function names and signatures differ for every artifact. Each artifact needs an adapter that binds the recovered task-level cases to its own API.
2. Write each adapter without looking at that artifact's test outcome. Freeze it and keep it auditable: the adapter changes only names and signatures, never inputs or expected values. A wrong binding counts as an adapter failure, not a model failure.
3. Tasks without usable checks in a language become `manual_semantics_only`, as `evaluation-scope-recommendation.md` already says. They stay out of aggregate functional pass rates.
4. Running tests means running generated code. That needs a sandbox: no network, a temporary filesystem, CPU, memory and time limits, and a Linux target for the POSIX C/C++ code. No such sandbox exists on this machine yet: WSL has no distro and Docker is not installed.
5. Passing gate 1 (compiles) never counts as functional correctness.

## Update (Milestone 2 catalog, stricter than the regex scan above)

A test counts as usable only if it has checks, binds to the original implementation rather than an embedded copy, and calls at least one symbol defined there. On that basis the catalog proposes a reference test for these task-language pairs:

| language | references | all models' tests agree | reference backed by only one of several tests |
|---|---|---|---|
| Python | 16 | 11 | 2 |
| Java | 33 | 14 | 13 |
| C | 11 | 6 | 0 |
| C++ | 16 | 9 | 3 |

Tasks with a reference in all four languages: 9 (3, 9, 12, 14, 18, 21, 23, 25, 31). Java references backed by only one of several tests reflect test sets that differ per model. Look at these first in review. Detail: `functional/oracle_catalog.csv`.
