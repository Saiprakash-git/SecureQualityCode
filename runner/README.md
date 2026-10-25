# MajorProject-VII — Experiment Runner

## Files

| File | Purpose |
|---|---|
| `run_experiment.py` | The main runner script |
| `run_config.json` | **Edit this to change the experiment** |
| `.env.example` | Template — copy to `.env.local` and add your key |

---

## Quick Start

### 1. Set up your API key

```powershell
# Option A: Environment variable (temporary, current session only)
$env:OPENROUTER_API_KEY = "sk-or-v1-your-rotated-key-here"

# Option B: .env.local file (persistent, loaded automatically)
Copy-Item .env.example .env.local
# Then edit .env.local and paste your key
```

> ⚠️ Rotate your exposed OpenRouter key first at https://openrouter.ai/keys  
> Then paste the **new** key here, not the old one.

### 2. Install dependency (optional but recommended)

```powershell
pip install python-dotenv
```

### 3. Choose a configuration

`run_config_pilot.json` and `run_config_main.json` use a `models` list and run every selected task-language combination for every listed model. `run_config.json` is retained as a single-model compatibility example.

Each API request asks for one language's raw source code. For 20 tasks, 4 languages, and 5 models, the runner schedules 400 combinations before retries. `max_attempts` means the total attempts for one combination, including the initial request. `retry_max` is retained only as a legacy fallback when `max_attempts` is absent.

Edit the exact `model_id` values after checking the current OpenRouter catalog. The labels in the pilot/main configs are placeholders and must not be treated as verified latest-model versions.

To edit task selection in the compatibility config:

```json
"task_selection": {
  "mode": "list",
  "task_ids": [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20]
}
```

Change `task_ids` to whatever tasks you want. Or switch `mode` to `"range"` and set `range_start`/`range_end`.

### 4. Do a dry run first (no API calls, no cost)

```powershell
cd f:\MajorProject-VII\runner
python run_experiment.py --dry-run
```

For the five-model pilot configuration:

```powershell
python run_experiment.py --config run_config_pilot.json --dry-run
```

### 5. Run the real experiment

```powershell
python run_experiment.py --config run_config_pilot.json
```

Run the main configuration only after reviewing the pilot ledger and replacing its placeholder model IDs:

```powershell
python run_experiment.py --config run_config_main.json
```

---

## Changing Task Count

| You want | What to do in `run_config.json` |
|---|---|
| **20-task pilot** | `"mode": "list"`, 20 IDs in `task_ids` |
| **60-task benchmark** | `"mode": "range"`, `"range_start": 1`, `"range_end": 60` |
| **85/90/100 tasks** | `"mode": "range"`, `"range_end": 85` (or 90, 100) |
| **All 200 tasks** | `"mode": "all"` |

## Changing the Model

Edit `model.model_id` in `run_config.json`:

```json
"model_id": "openai/gpt-4o"
```

Other options (all work through OpenRouter):
- `"anthropic/claude-3.5-sonnet"`
- `"google/gemini-pro-1.5"`
- `"meta-llama/llama-3.1-70b-instruct"` ← cheap/free
- `"deepseek/deepseek-coder"` ← code-specialized, cheap

## Output Structure

```
output/
└── pilot-run-001/          ← run_id from config
    ├── run_manifest.json   ← frozen config + task list
    ├── cost_ledger.jsonl   ← one line per task: tokens, cost, errors
    ├── run_summary.json    ← totals after run completes
    ├── run_YYYYMMDD.log    ← full log
    ├── Task_001/           ← per-task folder
      └── model-label/     ← one directory per configured model
        ├── Task1_model-label.py
        ├── Task1_model-label.java
        ├── Task1_model-label.c
        └── Task1_model-label.cpp
```

  Every run requires a fresh non-empty output directory; the runner refuses to append to an existing ledger. Each attempt has a raw response JSON record and one ledger row with the requested/actual model, request ID, finish reason, tokens, cost, response status, and validation status. This runner performs generation and source validation only. It does not run compilation, tests, SonarQube, CodeQL, dependency scanning, or semantic review. Those are separate evaluation stages described in the project runbook.
