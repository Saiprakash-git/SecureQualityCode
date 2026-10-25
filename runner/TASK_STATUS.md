# Pilot Task Status (20-task pilot, original dataset task IDs)

Last updated: 2026-10-26. Update this file after every run.

## Spending so far (real API calls)

| Run | Config | Cost (USD) | Notes |
|---|---|---:|---|
| one-task-python-real-2026-10-25 | Task 7, python only, 5 models | 0.20866 | Real run. 4 of 5 models succeeded. qwen3-8-max-0902 returned empty content twice. |
| pilot-2026-10-25-refresh-20tasks | 20 tasks x 5 models x 4 languages | 0.00 | **Dry run only.** No API calls. Not spend. |

Total real spend recorded so far: **$0.20866**. Remaining from the $5 cap: about $4.79. This is the sum of the Task 7 ledger. Check the OpenRouter dashboard too.

## Task status

Status key:
- `DONE (python only)`: the Task 7 run. Java, C, and C++ are not generated yet.
- `NOT RUN`: no real API call yet.

| Original ID | Prompt title | Status | In 10-task run? |
|---:|---|---|---|
| 7 | Login functionality in client-server architecture | DONE (python only, 4 of 5 models) | No (already run) |
| 104 | Buffer Handling | NOT RUN | Yes |
| 41 | Buffer Overflow | NOT RUN | Yes |
| 24 | Regex validate PIN code | NOT RUN | Yes |
| 37 | SQL Injection Prevention | NOT RUN | Yes |
| 38 | Cross-Site Scripting (XSS) | NOT RUN | Yes |
| 39 | Command Injection | NOT RUN | Yes |
| 42 | Authentication System | NOT RUN | Yes |
| 43 | Secure Session Management | NOT RUN | Yes |
| 46 | User Registration System | NOT RUN | Yes |
| 51 | Data Encryption | NOT RUN | Yes |
| 52 | File Encryption | NOT RUN | No (next batch) |
| 0 | Locking Mechanism | NOT RUN | No (next batch) |
| 36 | File Reading | NOT RUN | No (next batch) |
| 1 | Number of People in the Bus | NOT RUN | No (next batch) |
| 2 | Phone Number | NOT RUN | No (next batch) |
| 5 | Persistent Bugger | NOT RUN | No (next batch) |
| 6 | Find the unique number | NOT RUN | No (next batch) |
| 8 | Find the missing letter | NOT RUN | No (next batch) |
| 3 | Pangram | NOT RUN | No (next batch) |
| 4 | Untitled string task | NOT RUN | No (next batch) |

## Run plan

- **Batch 1 (10 tasks, 200 combinations):** `runner\run_config_pilot_10task_haiku_flash.json`, run ID `pilot-10-haiku-flash-2026-10-26`. Task IDs: 104, 41, 24, 37, 38, 39, 42, 43, 46, 51.
- **Batch 2 (remaining 9 of the 20 tasks, plus Task 7 for Java/C/C++ if you want it):** create a new config with a new run ID after batch 1 is reviewed.

## Rules

- Do not reuse a run ID. The runner refuses to write into an existing output folder.
- Do not change model IDs mid-comparison. If one changes, record it here first.
- Dry runs cost nothing, but they still create output folders. Delete or ignore them before a real run that uses the same ID.
