# Isolated Pilot Run Instructions

This package was created in a new folder. It does not modify the existing configs or output folders.

## Before running

1. Revoke any previously exposed OpenRouter key and create a new key.
2. Check the OpenRouter catalog and replace the five `model_id` values in `run_config_pilot_2026_10_23.json` with the exact model IDs you want to benchmark. The IDs currently copied into the file are configured historical IDs, not a claim that they are the latest models.
3. Confirm the price for each selected model and keep the hard budget at or below the approved amount.
4. Set the key only in the current PowerShell session:

```powershell
$env:OPENROUTER_API_KEY = "your-rotated-key"
```

Do not place the key in this folder, the JSON file, generated code, or log files.

## No-cost dry run

From the project root:

```powershell
cd F:\MajorProject-VII\runner
python run_experiment.py --config pilot_2026_10_23\run_config_pilot_2026_10_23.json --dry-run
```

Expected result:

- 20 selected tasks;
- 5 model loops;
- 100 model-task artifacts;
- 400 language source files;
- zero API requests;
- zero cost.

The isolated output will be created at:

```text
F:\MajorProject-VII\output\pilot-2026-10-23\pilot-2026-10-23-20tasks
```

## Real pilot run

Only after reviewing the dry-run summary and verifying model IDs:

```powershell
cd F:\MajorProject-VII\runner
python run_experiment.py --config pilot_2026_10_23\run_config_pilot_2026_10_23.json
```

The runner stores:

- `run_manifest.json`: frozen config and selected task IDs;
- `cost_ledger.jsonl`: one record per model-task request;
- `run_summary.json`: totals and success/error counts;
- one task/model directory containing the extracted Python, Java, C, and C++ files.

## Expected accounting

```text
20 tasks x 5 models = 100 API requests
100 responses x 4 languages = 400 expected language files
```

The API run is generation only. After it completes, do not treat generated files as validated. Run the separate evaluation funnel: parse/compile, tests, type checks, security analyzers, quality analyzers, sandbox checks, and sampled human review.

## Stop conditions

Stop and do not continue to the larger benchmark if:

- a model ID is unavailable;
- the provider reports missing or unusable cost metadata;
- the cost ledger does not contain one record per attempted request;
- output blocks are missing or mixed between languages;
- the budget is reached;
- generated programs cannot be safely sandboxed.

## Files in this package

- `run_config_pilot_2026_10_23.json`: isolated pilot configuration;
- `pilot_task_manifest.md`: exact task IDs, titles, and tags;
- this file: execution steps and stop conditions.
