# Evaluation pipeline

This folder is kept separate from `runner/`. The evaluator only reads the generation run folders. It never writes into them and never takes the runner's lock.

## Milestone 1: inventory and gate 1

```
python evaluation/evaluate.py --config evaluation/eval_config_benchmark_40.json --out evaluation/results/<name>
    [--tasks 0,3] [--workers 4] [--inventory-only]
```

Outputs go into a new folder, and the evaluator refuses a folder that is non-empty or inside a run folder:

| file | content |
|---|---|
| `inventory.csv/json` | One row for each of the 800 planned combinations: status, attempts, billed attempts, finish reason, source path and SHA-256. Meta: ledger hashes, whether generation was active, orphan sources, duplicate successes, unplanned rows. |
| `gate1_results.csv/json` | One row per combination: gate 1 status, integrity failures and warnings, syntax and build status, missing dependencies, first error. |
| `diagnostics/` | Full compiler output for each artifact that produced any, with temporary paths normalized. |
| `summary.json/md` | Funnel counts, pass rates with Wilson 95% CIs, model × language table, paired exact McNemar tests on complete task-language cells. |
| `eval_manifest.json` | Evaluator and helper hashes, git HEAD, tool versions, flags, input ledger hashes, timestamp. |

Running twice on the same inputs gives byte-identical CSV and JSON. The exceptions are the manifest timestamp and the `summary.md` title, which is the folder name.

### Inventory statuses
- `generated`: an accepted source exists.
- `generated_truncated`: an accepted source exists, but `finish_reason=length`.
- `failed`: all billed attempts were used and no source was accepted.
- `incomplete`: attempts remain (a run that stopped or is still in progress).
- `not_attempted`: no attempts yet.
- `source_missing`: the ledger records a success, but the file is absent.

### Gate 1 statuses
Listed in precedence order:
1. `fail_integrity`: empty file, truncated, or leftover Markdown fence.
2. `toolchain_error`
3. `fail_syntax`: Python `ast` and `compile`, or the Java parse-only helper.
4. `blocked_dependency`: a third-party header or package is missing.
5. `fail_compile`
6. `pass`

`no_artifact` means there is no source to check. A truncated file is `fail_integrity` even if it compiles; this is counted separately.

### Checks and isolation
- **Python:** `ast.parse` plus `compile(..., dont_inherit=True)`. Syntax only. Imports are listed but not resolved.
- **Java:** a parse-only check (`tools/JavaParseCheck.java`, JDK compiler API), then `javac -proc:none -implicit:none` with an empty classpath and sourcepath. The file is copied under its public type's name, because javac requires that. This is the only change made before compiling, and it is recorded as `java_file_name`.
- **C/C++:** clang 20 via zig 0.15.2. Target `x86_64-linux-gnu` (glibc headers), `-std=gnu17` / `-std=gnu++17` (GCC's defaults), `-Wall`, compiled to an object file only, no linking. The generated code uses POSIX APIs, so a Linux target avoids counting Windows platform errors as code defects. MSVC is installed but was not used for this reason.
- **Execution:** none. Every tool runs on a copy in a fresh temporary folder, with a timeout (120 s), stdin closed, and a reduced environment (no API keys, CLASSPATH, JAVA options or include-path variables).

### Toolchain setup (once)
```
python -m venv evaluation/.toolchain
evaluation/.toolchain/Scripts/python -m pip install ziglang==0.15.2
```
You also need JDK 21 `javac` and `java` on PATH. `.toolchain/` is git-ignored.

### Tests
```
python evaluation/tests/test_evaluate.py -v
```

## Milestone 2: functional correctness (framework; nothing has been executed on generated code)

```
python evaluation/upstream_fetch.py        # pinned upstream tests + original implementations -> .upstream/ (git-ignored)
python evaluation/functional.py catalog    # -> functional/oracle_catalog.json/.csv
python evaluation/functional.py plan --gate1 <gate1 results> --out <new folder> [--workspaces]
python evaluation/tests/test_functional.py -v
```

**Oracle catalog.** For each upstream test file, the catalog records:
- check count and framework;
- binding: `import_original`, `include_original`, `class_reference`, `embedded_reference` (the test contains its own copy of the code, so it is not usable), `none` or `empty`;
- the canonical symbols it calls, with signatures read from the same model's original implementation;
- a fingerprint of its literals (inputs and expected values).

For each task and language, the proposed reference test is the largest group of usable tests with the same fingerprint. Ties are broken by model order. `agreement` reports that group's size.

**Approval.** A reference test is used only after a human records approval in `functional/oracle_review.json`:

```json
{ "18/c": {"approved": true, "test_sha256": "<reference test_sha256>", "reviewer": "name", "notes": "..."} }
```

An approval is ignored if the test file's hash changes.

**Adapters.** Adapters only bind names. A canonical symbol is mapped to an artifact symbol:

| match | result |
|---|---|
| same name and arity (`alias_exact`) | `auto_approved` |
| name equal ignoring case and `_` (`alias_normalized`) | `draft` |
| any binding to `main` | `draft` |
| no unique candidate | `unresolved` |

Only `auto_approved` or human-`approved` adapters whose artifact and test hashes still match are used.

What a workspace changes:
- **Python, C, C++:** the test's binding lines are replaced by one adapter line. The artifact is copied byte for byte.
  - Python: `from impl_under_test import <target> as <canonical>`.
  - C/C++: an `adapter.h` that renames the artifact's `main`, includes the artifact, and `#define`s canonical names.
- **Java:** the test is unchanged. The artifact copy gets the test's `package` line. A facade class with the canonical class name delegates static calls.

Every change is listed in `workspace.json`.

**Result categories.**

| category | meaning |
|---|---|
| `passed` | all checks pass |
| `failed` | at least one assertion or FAIL check fails |
| `error` | build failure, crash, import error, or exceptions without any assertion failure |
| `timeout` | a build or the test run exceeded its limit |
| `not_testable` | reason recorded: `gate1:*`, `no_oracle`, `oracle_unreviewed`/`rejected`, `adapter: ...`, `sandbox_unavailable` |

**Execution.**
- `NoSandbox` is the default, so everything is `not_testable`.
- `TrustedFixtureExecutor` runs only the framework's own fixtures (hash allowlist) on the host, for the tests.
- `WslBwrapExecutor` (`plan --execute`) runs each workspace in a fresh bubblewrap sandbox inside the `eval-sandbox` WSL distro. Setup, versions and verification are in `sandbox/SANDBOX_STATE.md`. It runs only when all of these hold:
  - the sandbox self-test (`sandbox/check_hardening.sh` + `sandbox/check_bwrap.sh`) passes at start-up;
  - every workspace file matches its manifest hash;
  - the oracle and the adapter both carry a human approval, and the adapter names a reviewer.

## Human review (nothing is approved by the pipeline)

```
python evaluation/functional_review.py --plan evaluation/results/<functional plan folder>
```

This writes `functional/review/ORACLE_REVIEW.md` and `ADAPTER_REVIEW.md`, both git-ignored because they quote unlicensed upstream tests, plus JSON templates with every decision `null`.

Reviewers copy decided entries into these files, keeping the recorded hashes:
- `functional/oracle_review.json`: `{"<task>/<lang>": {"approved", "test_sha256", "reviewer", "notes"}}`
- `functional/adapter_review.json`: `{"<combo_index>": {"approved", "reviewer", "artifact_sha256", "oracle_test_sha256", "bindings"}}`

Proposal labels (`proposed_exact`, `proposed_loose`, `unresolved`) are review hints, not approvals.

Test-suite provenance and coverage: `test_recovery/TEST_SUITE_RECOVERY.md`.
