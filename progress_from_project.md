# MajorProject-VII: Progress Handover

This file explains the project state, the code, how runs are configured and executed, and how to resume safely. Read it before changing the runner or spending money on the OpenRouter API.

Last updated: 2026-10-26, after the resume run finished.

---

## 1. What the project is

A research benchmark on the security and quality of LLM-generated code.

- **Dataset:** task JSON files in `Repo\SecureQualityCode\code-llm-evaluation-dataset-main\code-llm-evaluation-dataset-main\dataset\promptDescription-0XX` and `promptDescription-1XX` (100 files each). Each file holds one task with `task_number`, `prompt_title`, `prompt_description`, `tags`, and so on.
- **Generation:** the runner sends each task, in each language (python, java, c, cpp), to each configured model through OpenRouter. It saves the returned source code and every attempt's raw response.
- **Goal:** compare models on correctness, security, quality, cost, and repairability. The pilot is a cost and reliability check, not the benchmark itself. See `runner\pilot_2026_10_23\pilot_and_remaining_90_tasks.md`.

Key planning docs: `research-agenda.md`, `benchmark-runbook.md`, `runner\README.md`.

---

## 2. Current state (as of 2026-10-26)

### 2.1 Spend

| Run (output folder) | Recorded cost (USD) | Notes |
|---|---:|---|
| `output\one-task-python-real-2026-10-25` | 0.20866 | Task 7, python only, 5 models. Qwen (`qwen3-8-max-0902`) returned empty content twice. |
| `output\pilot-10-haiku-flash-2026-10-26\pilot-10-haiku-flash-2026-10-25-real` | 0.637065 | Interrupted after combination 120 of 200 (power outage / Wi-Fi). |
| `output\pilot-10-resume-2026-10-26\pilot-10-resume-from-121-2026-10-26-real` | 1.252278 total | Combinations 121 to 200. Its summary starts from the carried-over 0.637065, so the **new spend is 0.615213**. |

**Recorded total spend: $1.252278 of the $5.00 cap.**

> Reconcile every figure with the OpenRouter Activity dashboard before relying on it. Unknown-cost requests (for example the `IncompleteRead` on combination 120) are not in the recorded total.

### 2.2 Pilot task status

The 10-task pilot covers original task IDs 104, 41, 24, 37, 38, 39, 42, 43, 46, 51. Task 7 was run separately and is not in the 10. The full 20-task list is in `runner\pilot_2026_10_23\pilot_task_manifest.md`.

Per-task status is in `runner\TASK_STATUS.md`. That file was written before the resume run, so it may be out of date.

### 2.3 Combinations not generated (excluded on purpose)

| Combination (1-based) | Task | Language | Model | Reason |
|---:|---:|---|---|---|
| 55 | 38 | c | qwen3-8-flash | empty content, 3 attempts |
| 60 | 38 | cpp | qwen3-8-flash | empty content, 3 attempts |
| 114 | 42 | c | mistral-large-4 | empty content, 3 attempts |
| 115 | 42 | c | qwen3-8-flash | empty content, 3 attempts |
| 119 | 42 | cpp | mistral-large-4 | empty content, 3 attempts |
| 120 | 42 | cpp | qwen3-8-flash | `IncompleteRead`, cost unknown, may have been billed |

The resume run skipped these deliberately. Retry them only after reconciling spend and understanding the cause.

### 2.4 Resume run outcome

The resume run's log has 80 progress lines (`[121/200]` to `[200/200]`), so every combination from 121 onward was attempted. Its summary reports `tasks_success: 70` and `tasks_error: 10` out of 80 attempted. The 10 errors must be listed from that run's ledger before any retry (section 6). I checked this against the ledger: 80 unique combinations appear, 70 have a `success` row with a saved source file, and 10 do not. All 10 failures are Task 43 (`10` of the 80 combinations), listed below. Tasks 46, 51, and 104 had some failed attempts, but each of those combinations eventually succeeded on a retry or on the first try.

**Failed combinations in the resume run (not generated):**

| Task | Language | Model | Attempts | Last status |
|---:|---|---|---:|---|
| 43 | python | mistral-large-4 | 3 | empty_content |
| 43 | python | qwen3-8-flash | 3 | empty_content |
| 43 | java | claude-haiku-5-5 | 3 | empty_content |
| 43 | java | qwen3-8-flash | 3 | empty_content |
| 43 | c | claude-haiku-5-5 | 3 | empty_content |
| 43 | c | mistral-large-4 | 3 | empty_content |
| 43 | c | qwen3-8-flash | 3 | empty_content |
| 43 | cpp | claude-haiku-5-5 | 3 | empty_content |
| 43 | cpp | mistral-large-4 | 3 | empty_content |
| 43 | cpp | qwen3-8-flash | 3 | empty_content |

The table has exactly 10 rows, matching the script's count of combinations with no `success` row. Python Task 43 with `gemini-3-8-flash` and `claude-haiku-5-5` did succeed (their ledger rows include a `success`), so they are not failures. The ledger, checked by script, is the source of truth for any retry.

---

## 3. Code: `runner\run_experiment.py`

Single-file runner. Entry point: `main()`.

| Function | Role |
|---|---|
| `load_config` | Reads a JSON config. |
| `load_all_tasks` | Loads every task JSON from `dataset_dirs`, keyed by `task_number`. |
| `select_tasks` | Selects tasks. Modes: `all`, `range`, `orig_id_list`, `list`. Any other mode raises `ValueError`. |
| `validate_preflight` | Checks that configured task IDs exist and are unique, and that counts match `expected_task_count` and `expected_combinations`. Runs before any output or API call. |
| `call_openrouter` | One HTTP POST to `/chat/completions`. Raises `OpenRouterResponseError` with the raw payload on failure. |
| `_usage_values` | Reads tokens and `total_cost` from a response. |
| `_response_content` | Extracts message content, or returns a status such as `empty_content`. |
| `generate` | One combination. Up to `max_attempts` attempts (the first request counts). Stops on a valid source, unknown cost, or budget exhaustion. Calls `on_attempt` after each attempt. |
| `normalize_source_response` | Validates output. Python uses `ast.parse`. Java, C, and C++ use a heuristic. Rejects refusals and prose. Accepts one matching outer code fence. |
| `generation_plan` | Yields combinations in order **task -> language -> model**, so a budget stop covers all models on the same early tasks. |
| `_write_new_file` | Writes a file only if it does not exist (mode `"x"`). |
| `prepare_output_dir` | Refuses a run folder that already contains files. |
| `main` | Preflight, then the loop, then the summary. |

### 3.1 Safety features

- **Preflight before any API call** (`validate_preflight`).
- **Budget cap** (`budget_usd_hard_limit`), checked before each combination and each retry. One in-flight request can still exceed the remaining budget.
- **Immediate persistence.** `persist_attempt` writes the raw attempt JSON and one ledger line, then flushes and fsyncs. A crash loses at most the request in flight.
- **No double counting.** Usage and cost are added to the totals once, when the response arrives. Later processing errors keep the figures on the attempt record only.
- **Unknown cost stops the run.** A response with no cost metadata halts further paid calls.
- **No overwrites.** Run folders must be new, and files are created in exclusive mode.

### 3.2 Resume support

Two optional config keys:

- `start_combination` (1-based, default 1). Combinations before it are skipped with no API call. Must be between 1 and the planned total.
- `prior_spend_usd` (default 0). Sets the starting running total, so the $5 cap includes earlier runs.

The skip happens in `main()`: `enumerate(generation_plan(...), start=1)` followed by `continue`. The summary records both values.

**Note:** the progress counter `[n/200]` uses the full plan size. A resumed run starting at 121 prints `[121/200]` first.

### 3.3 Known limits

- Refusals, prose, and empty responses are recorded as failures and are not retried indefinitely.
- The Java, C, and C++ check is a heuristic. Compilation is not part of this stage.
- `retry_max` is a legacy key. `max_attempts` controls retries and includes the first request.
- Section 2.4's failure count has been checked against the ledger.

---

## 4. Configs: which file does what

All configs are in `runner\`. Never edit a config that has already produced output. Copy it, give it a new `run_id`, and change only what you need.

| File | Purpose |
|---|---|
| `run_config_pilot.json` | Original 20-task pilot config with placeholder model IDs. Not used for a paid run. |
| `run_config_main.json` | Main benchmark config. Not yet used. Placeholder model IDs. |
| `run_config_pilot_20task_haiku_flash.json` | 20-task pilot with the Haiku/Flash models. Run ID `pilot-20-haiku-flash-2026-10-26`. Not run. |
| `run_config_pilot_10task_haiku_flash.json` | 10-task config (run ID `pilot-10-haiku-flash-2026-10-26`). Interrupted at combination 120 of 200. |
| `run_config_pilot_10task_resume.json` | Resume config: `start_combination: 121`, `prior_spend_usd: 0.637065`. Ran 80 combinations. |
| `pilot_2026_10_25_refresh\run_config_pilot_2026_10_25_refresh.json` | Source of the Haiku/Qwen model list. Only a dry run was done with it. |
| `pilot_2026_10_23\run_config_pilot_2026_10_23.json` | Earlier pilot config. |
| `tests\fixtures\*.json` | Dry-validation fixtures used by the test suite. |

Model IDs in the Haiku/Flash pilots: `anthropic/claude-haiku-5.5`, `openai/gpt-6-luna`, `google/gemini-3.8-flash`, `mistralai/mistral-large-4-0`, `qwen/qwen3.8-flash`. They came from earlier runs and have not been checked against the live OpenRouter catalog.

---

## 5. How to run

Set the key for the current PowerShell session only:

```powershell
$env:OPENROUTER_API_KEY = "your-key"
cd F:\MajorProject-VII\runner
```

Dry-run first (no API calls), then the real run:

```powershell
python run_experiment.py --config <config>.json --dry-run
python run_experiment.py --config <config>.json
```

Tests: `python tests\test_run_experiment.py`. Last result: **34 of 34 pass.**

### 5.1 Rules for a new run

1. Use a **new `run_id`** every time. The runner refuses a non-empty output folder.
2. **Dry-run into a temp folder** first, or delete the dry-run folder. Dry runs still create `output_dir\run_id`.
3. **Keep models and parameters fixed** within one comparison.
4. **Set `prior_spend_usd`** to the total spent so far, so the cap covers all runs.
5. **Set `expected_combinations`** so preflight catches mistakes.
6. **Record the run** in `runner\TASK_STATUS.md`.

### 5.2 Reading a run's results

- `run_summary.json`: totals and whether the run stopped early.
- `cost_ledger.jsonl`: one line per attempt, with status, cost, and raw response path.
- `Task_NNN\<model>\`: source files and `*_attempt_NN_response.json` files.
- `run_*.log`: console log with the same progress lines.

### 5.3 Unsafe actions

- Do not restart a run with the same `run_id`.
- Do not remove `start_combination` from a resume config. Without it, the whole plan runs again and costs money a second time.
- Do not raise `budget_usd_hard_limit` to get around the cap.
- Do not run the CodeQL extractor until its hardcoded GitHub token is removed (section 7).

---

## 6. Recovering from failures

**Power loss or crash:**
1. Do not rerun the same `run_id`.
2. Reconcile the ledger with the OpenRouter Activity page for the run's time window.
3. Find the last `[n/200]` line in the log. The request in flight at that point may be billed with no ledger line.
4. Build a new config with `start_combination` set to the next unfinished combination and `prior_spend_usd` set to the reconciled total.

**Listing failed combinations:** count unique `(task_id, language, model_label)` keys, not ledger lines. A combination is complete if it has a `success` row and a saved source file. One combination can have up to 3 ledger rows.

**Empty responses** from `qwen3-8-flash`, `mistral-large-4`, and `claude-haiku-5-5` come from the model returning no content, not from the runner. Retry manually only after the cause is understood.

---

## 7. Security and open issues

1. **OpenRouter key** `sk-or-v1-...` was stored in a plain text file in the project root. The file is gone from the workspace, but the key is still in Git history (commit `72b1c4c`). **Rotate it** on the OpenRouter dashboard. Rewriting history does not undo exposure.
2. **CodeQL extractor** has a hardcoded GitHub PAT (see `runner\pilot_2026_10_25_refresh\PILOT_REFRESH_NOTES.md`). Revoke it and move the token to an environment variable before running that script.
3. **SonarQube token** in an older properties file. Revoke it.
4. **Model IDs** in the pilot configs are unverified against the live catalog.
5. **Uncommitted changes** to `run_experiment.py`, the configs, the tests, and this document. Commit after review.
6. **Dry-run folders** in `output\` can collide with real run IDs. Keep them separate.

---

## 8. Changes made (chronological)

1. **Syntax fix** in `select_tasks`: removed a misplaced `else` block; added an explicit `list` mode and an error for unknown modes.
2. **Preflight wired into `main()`**, before any output folder or API call.
3. **Budget set to $5.0** in the pilot config. Other settings unchanged.
4. **Per-attempt persistence:** `generate()` takes `on_attempt`; `main()` writes each attempt immediately and flushes the ledger.
5. **Exclusive file writes** (`_write_new_file`), so outputs are never overwritten.
6. **Processing-error accounting fixed:** cost and tokens stay on the attempt record and are counted once in the totals.
7. **Generation order changed** to task -> language -> model.
8. **Regression tests** for the brace regex and the dry run.
9. **Resume support:** `start_combination` and `prior_spend_usd`, with the summary recording both.
10. **Resume config** `run_config_pilot_10task_resume.json`.

The source-validation regex `[*{};()]*` was not in the file on disk. The current pattern is `[{};()]`, and a regression test covers it.

---

## 9. Tests

File: `runner\tests\test_run_experiment.py`.

- `RunnerTests`: generation, fences, empty responses, budget, unknown cost, API errors, missing choices, output-folder protection.
- `PilotConfigPreflightTests`: real pilot config and dataset. 20 unique tasks, 400 combinations, $5 budget, old run ID not reused.
- `RegressionTests`: brace regex.
- `DryRunTests`: no API call, empty ledger.
- `GenerationOrderTests`: task -> language -> model order and budget coverage.
- `PersistenceTests`: `on_attempt` fires for each attempt; `_write_new_file` refuses overwrites.
- `ResumeTests`: resume point, 80 placeholders only, invalid start rejected, carried-over spend blocks new calls.

Run them before and after any runner change.

---

## 10. Next steps

1. Reconcile all spend against OpenRouter Activity (recorded total $1.252278).
2. Decide whether to retry the 10 Task 43 failures in section 2.4. Empty-content failures are common for `qwen3-8-flash`, `mistral-large-4`, and `claude-haiku-5-5`.
3. Decide on the six excluded combinations (55, 60, 114, 115, 119, 120).
4. Rotate the exposed keys (section 7).
5. Verify the model IDs against the live catalog before any new paid run.
6. Commit the reviewed changes to Git.
7. Update `runner\TASK_STATUS.md` with the final figures.
8. Plan the remaining 10 tasks of the 20-task pilot with a new config and a new run ID.
