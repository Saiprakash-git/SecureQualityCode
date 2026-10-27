# Reproducing the benchmark-40 evaluation

Run from the repository root. Each step writes to a new folder and refuses to overwrite. Raw generation outputs under `output/` are only read.

## 0. One-time setup (already done on this machine)

```
python -m venv evaluation/.toolchain && evaluation/.toolchain/Scripts/python -m pip install ziglang==0.15.2   # gate 1 C/C++
# WSL sandbox (eval-sandbox), toolchains, JUnit and scanners: see evaluation/sandbox/SANDBOX_STATE.md
python evaluation/upstream_fetch.py               # pinned upstream tests -> evaluation/.upstream/
python evaluation/functional.py catalog           # -> evaluation/functional/oracle_catalog.json
```

## 1. Snapshot and gate 1 (integrity, syntax, compile)

```
python evaluation/evaluate.py --config evaluation/eval_config_benchmark_40.json --out evaluation/results/benchmark-40-final-gate1
```

The snapshot manifest is `evaluation/results/SNAPSHOT_benchmark-40-final.json` (snapshot id `30d64f1cb11b3160…`, 734 artifacts, hashes re-verified).

## 2. Review, then functional tests in the sandbox

The reviews are already recorded in `evaluation/functional/oracle_review.json` and `adapter_review.json`.

```
python evaluation/functional_review.py --plan <a plan folder>      # review reports (optional)
python evaluation/functional.py plan --gate1 evaluation/results/benchmark-40-final-gate1 \
       --out evaluation/results/benchmark-40-final-functional-v2 --execute
```

`--execute` first runs the sandbox security self-test (31 checks). It then runs only the combinations whose oracle and adapter are approved.

## 3. Static security and quality scans

```
python evaluation/scan.py --snapshot evaluation/results/SNAPSHOT_benchmark-40-final.json \
       --gate1 evaluation/results/benchmark-40-final-gate1 --out evaluation/results/benchmark-40-final-scans
```

## 4. Consolidated results

```
python evaluation/consolidate.py --gate1 evaluation/results/benchmark-40-final-gate1 \
       --functional evaluation/results/benchmark-40-final-functional-v2 \
       --scans evaluation/results/benchmark-40-final-scans \
       --snapshot evaluation/results/SNAPSHOT_benchmark-40-final.json \
       --out evaluation/results/benchmark-40-final-consolidated
```

Outputs:
- `RESULTS.md` (tables)
- `summary.json`
- `table_by_model.csv`, `table_by_model_language.csv`
- `combined_per_combination.csv/json` (800 rows with separate evidence columns)

## Tests

```
python evaluation/tests/test_evaluate.py; python evaluation/tests/test_functional.py; python evaluation/tests/test_wsl_sandbox.py
```
