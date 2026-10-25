# One-Task, One-Language Smoke Test

This is a new isolated smoke-test folder. It does not modify the existing pilot configuration or output.

## What it runs

- Original dataset task ID: `7`
- Prompt title: `Login functionality in client-server architecture`
- Language: Python only
- Models: all five configured OpenRouter models

Expected real workload:

```text
1 task x 5 models x 1 language = 5 API requests
5 responses x 1 language = 5 Python files
```

The task prompt still contains the security-sensitive login/buffer-overflow context. This smoke test checks the runner, model IDs, API key, response extraction, and per-model storage. It is not the full semantic/security evaluation.

## Step 1: Validate locally without API cost

From PowerShell:

```powershell
cd F:\MajorProject-VII\runner
python -m json.tool dry_run_one_task_2026_10_23\run_config_one_task_python.json | Out-Null
python run_experiment.py --config dry_run_one_task_2026_10_23\run_config_one_task_python.json --dry-run
```

Expected dry-run summary:

```text
Tasks loaded: 200 total, 1 selected
Model [1/5] through Model [5/5]
DONE - 5/5 artifacts succeeded
Total cost: $0.0000
```

The dry-run output is placed at:

```text
F:\MajorProject-VII\output\one-task-python-smoke-2026-10-23\one-task-python-smoke-2026-10-23
```

## Step 2: Prepare the real smoke test

1. Confirm that all five model IDs in `run_config_one_task_python.json` are valid and available in OpenRouter.
2. Make sure the API key is rotated and is not stored in this folder or any source file.
3. Set the key in the same PowerShell window used for the real command:

```powershell
$env:OPENROUTER_API_KEY = "your-rotated-key"
```

4. Verify key presence without printing it:

```powershell
-not [string]::IsNullOrWhiteSpace($env:OPENROUTER_API_KEY)
```

Expected result:

```text
True
```

5. Confirm that the output directory does not already exist. Use a new `run_id` and `output_dir` for every real attempt.

## Step 3: Run the real smoke test

```powershell
cd F:\MajorProject-VII\runner
python run_experiment.py --config dry_run_one_task_2026_10_23\run_config_one_task_python.json
```

Do not include `--dry-run` in the real command.

## Step 4: Check the result

Inspect:

```text
F:\MajorProject-VII\output\one-task-python-smoke-2026-10-23\one-task-python-smoke-2026-10-23
```

Check these files:

- `run_manifest.json`
- `cost_ledger.jsonl`
- `run_summary.json`

Expected real-run accounting:

- 5 ledger entries;
- 5 successful model-task artifacts if every response contains a Python code block;
- 5 Python source files;
- actual token counts and costs;
- no model/API errors.

## Stop conditions

Stop before the larger pilot if:

- any model ID returns a model-not-found error;
- the cost exceeds the configured budget;
- usage or cost metadata is missing;
- a response has no Python code block;
- the ledger does not contain five records;
- the output contains secrets or unsafe hard-coded credentials.

Generated code is untrusted. Do not execute it on the host until it is placed in a restricted sandbox.
