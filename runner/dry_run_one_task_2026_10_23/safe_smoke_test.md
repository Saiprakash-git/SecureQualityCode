# Safe One-Task Smoke Test

The previous real smoke test exposed two behaviors:

- Claude and Mistral reached the 4,096-token output limit before closing the Python fence.
- One response returned null content, which the original runner did not handle.

This new entry point does not overwrite the existing runner. It treats null/non-text responses as recorded errors and continues. It uses 8,192 maximum output tokens for the one-language test.

## No-cost validation

```powershell
cd F:\MajorProject-VII\runner
python -m json.tool dry_run_one_task_2026_10_23\run_config_one_task_python_safe.json | Out-Null
python dry_run_one_task_2026_10_23\run_experiment_safe.py --config dry_run_one_task_2026_10_23\run_config_one_task_python_safe.json --dry-run
```

## Real run

Set the rotated key in the same PowerShell session:

```powershell
$env:OPENROUTER_API_KEY = "your-new-rotated-key"
```

Then run:

```powershell
python dry_run_one_task_2026_10_23\run_experiment_safe.py --config dry_run_one_task_2026_10_23\run_config_one_task_python_safe.json
```

Output:

```text
F:\MajorProject-VII\output\one-task-python-smoke-safe-2026-10-24\one-task-python-smoke-safe-2026-10-24
```

Expected accounting:

- 5 model-task attempts;
- 5 ledger records;
- successful Python files only for responses containing a complete fenced Python block;
- error files for null, truncated, or malformed responses;
- a final `run_summary.json` even when one model fails.
