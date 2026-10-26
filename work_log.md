# Work Log

Chronological record of completed steps. Newest at the bottom.

## 2026-10-26

### 1. Project orientation (read-only)
- **Done:** Reviewed the project docs (`progress_from_project.md`, `research-agenda.md`, `evaluation-scope-recommendation.md`, `runner/BENCHMARK_40_SELECTION.md`, `runner/TASK_STATUS.md`), the runner, and the outputs.
- **Files changed:** none.
- **Checks:** Parsed `output/benchmark-40/benchmark-40-2026-10-26-real/cost_ledger.jsonl`: 160 unique combinations, 152 succeeded, 8 failed (7 empty-content, 6 of those from qwen3-8-flash, 1 from mistral-large-4; 1 HTTP 429). The recorded running total is $2.080105, including $1.460938 from earlier runs. The run stopped at combination 160 of 800 because the 429 response had no cost metadata (`runner/run_experiment.py:754`).
- **Unresolved:**
  - A 429 stops the whole run instead of backing off and retrying.
  - benchmark-40 needs a resume from combination 160 with `prior_spend_usd` 2.080105.
  - qwen3-8-flash returns empty content often.
  - The paper framing (CWE + business-logic contract) differs from the agenda (detect, repair, verify loop).
  - Exposed keys in Git history need rotating.
  - `progress_from_project.md` and `runner/TASK_STATUS.md` are out of date.
  - benchmark-40 files and `Latex_paper.txt` are not committed.

### 2. Adopted working rules
- **Done:** Recorded the user's rules: minimize tokens, plan and stick to the plan, keep this log, verify before claiming success.
- **Files changed:** `work_log.md` (created).
