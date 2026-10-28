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

### 3. Project review (read-only)
- **Done:** Reviewed the runner, tests, configs, research docs, the paper draft, `Major Project.pdf`, and both reference papers.
- **Files changed:** none. pypdf was installed into the session scratchpad only, for PDF text.
- **Findings:**
  - The 429 stop was caused by unknown cost.
  - All 96 empty responses had `finish_reason=length`, meaning reasoning used up the 4096-token limit.
  - 32 accepted outputs may be truncated.
  - Top-level temperature and top_p were ignored.
  - The resume progress counter was wrong.
  - The paper and plan conflict: the abstract says "single-pass", the draft and agenda add repair loops.

### 4. Runner: concurrency, backoff, budget reservations, key-based resume
- **Investigation (from existing ledgers):**
  - Empty responses: qwen 63, mistral 20, haiku 11, all hitting the 4096-token limit.
  - Retries after an empty response do recover some: qwen 17 of 31, mistral 2 of 8, haiku 2 of 5.
  - Empty attempts cost $0.59 of the $2.08 spent.
  - Worst observed cost per request: mistral $0.0177, gemini $0.0155, others under $0.004. These set `reserve_usd` at about 1.5 times the maximum.
- **Changed `runner/run_experiment.py`:**
  - **Worker pool** (`concurrency`). Threads, not asyncio, because the HTTP client is blocking urllib and no new dependency is needed. Each provider gets its own job cap, minimum gap between requests, and a cooldown shared by all workers (`rate_limits`).
  - **HTTP 429/5xx retries** with exponential backoff and jitter, honoring `Retry-After`. These retries don't count as generation attempts. A 429 is treated as not billed. A 5xx is charged provisionally against the budget.
  - **`BudgetTracker`.** Every request reserves `reserve_usd` before it is sent, so requests in flight count toward the cap. An unknown cost keeps the reservation as provisional spend and pauses all new dispatch.
  - **Key-based resume** (`resume_from_runs`), driven by the ledgers. A combination counts as finished if it has a saved source or has used all billed attempts. Otherwise it continues at the next attempt number.
  - **Resume safety checks.** The resume refuses: a success row without its source file, a changed model ID, an unclassifiable unknown-cost row, and a `prior_spend_usd` that differs from the resumed runs.
  - **Duplicate protection.** An exclusive `.runner.lock` in the output folder, plus a ledger writer that serializes and fsyncs writes.
  - **Ctrl+C.** Stops new dispatch, waits for in-flight work and saves it, writes the summary, and releases the lock.
  - **Config.** Top-level temperature, top_p and max_tokens are now honored, and a model entry overrides them. The API key is checked before any output is written.
  - **Tracking.** Ledger rows gain `combo_index`, `billing`, `provisional_cost_usd`, `truncated` and `transport_retries`. Progress shows this run's count, the plan index, the overall finished count, and spend plus in-flight reservations.
  - **Compatibility.** `start_combination` and the `generate()` keyword interface still work.
- **Added `runner/run_config_benchmark_40task_resume.json`.** Same tasks, models, languages, settings, budget and `prior_spend_usd` as `run_config_benchmark_40task.json`. New run ID `benchmark-40-2026-10-26-resume-01`. Adds `resume_from_runs` (the -real folder), concurrency 3, and qwen min interval 4 s. The original config is unchanged.
- **Added 23 tests to `runner/tests/test_run_experiment.py`:**
  - concurrency limits;
  - retries (Retry-After, 5xx backoff, persistent 429, no backoff on 400);
  - budget (reservations under contention, in-flight cap, unknown-cost pause, missing key);
  - resume and duplicates (resume only unfinished work, Ctrl+C then resume, lock, missing source, changed model, unknown row, spend mismatch);
  - config and tracking;
  - the real benchmark-40 resume plan.
- **Checks:**
  - 57 of 57 tests pass, three runs in a row.
  - Dry run of the resume config, into the scratchpad only, with no API calls: 641 combinations scheduled, starting at #160 (Task 18, C++, qwen, attempt 2); 159 skipped; carried-over total $2.080105.
  - Real `output/benchmark-40` unchanged: ledger last modified 15:59:23, no new folder, no lock.
- **Not changed (recommendation only):** generation settings (max_tokens 4096, max_attempts 3).
- **Unresolved:**
  - The OpenRouter key must be rotated before the real run.
  - Spend should be reconciled on OpenRouter Activity.
  - Model IDs are unverified.
  - The 429-not-billed assumption needs confirming on Activity.
  - Empty and truncated output policy after the benchmark: see the recommendation in the report.

### 5. Pre-run verification of the benchmark-40 resume (no API calls)
- **#160 and duplicates.**
  - Real ledger: attempt 1 billed $0.00197807 (empty); attempt 2 was an HTTP 429 with no response ID or cost. The resume continues at attempt 2, so at most 2 more billed attempts.
  - 185 ledger rows match 185 raw files. 152 source files match 152 success rows. No orphans, no duplicate attempt rows, no combination over 3 billed attempts.
  - **Gap fixed:** the lock only covered `output_dir`. It now also locks the parent folder of every resumed run, and a refusal releases any locks already taken.
- **Spend.**
  - 1.460938 = 0.20866 (one-task) + 0.637065 (pilot-10) + 0.615213 (pilot-10 resume), matching the ledgers. Adding the benchmark-40 ledger (0.619167) gives 2.080105, with no double counting.
  - **Issue found:** `output/one-task-python-smoke-2026-10-23` holds $0.072997 of real spend. An older runner wrote two real runs (Oct 23 19:47, Oct 24 19:48) into a dry-run folder, and the summary still says $0. Both runs stopped with a request in flight and no ledger row (gpt-6-luna, qwen3-8-max-0902). The pilot-10 `IncompleteRead` is also uncounted.
  - **Fix:** new config field `unreconciled_spend_usd` (counts toward the cap only), set to 0.13 = 0.072997 + upper bounds 0.04966 + 0.00353 + 0.00201, rounded up. Cap basis is now $2.210105.
- **Reservations.**
  - Per-token prices come from `cost_details` in the raw responses; the fee ratio is 1.0.
  - Longest prompt across the 40 tasks: at most 1,078 tokens. No response has ever gone over 4096 completion tokens.
  - Worst case per request vs `reserve_usd`: haiku 0.00216 / 0.0035; gpt 0.00353 seen / 0.006; gemini 0.01617 / 0.025; mistral 0.01859 / 0.03; qwen 0.00209 / 0.0035. All pass.
- **Retries.** At most 1 + `http_retry_max` (5) HTTP requests per attempt, then the combination stops as `rate_limited` (still resumable). Backoff is capped at 60 s × 1.25 jitter, and `Retry-After` at 300 s. A 5xx is charged provisionally each time. Tests added.
- **Credentials and output.**
  - No full-length OpenRouter key, GitHub token or Sonar token in the working tree or any Git commit. `runner/.env.example` holds only a placeholder. `.env.local` is gitignored.
  - **Issue found:** `.gitignore` contained a 12-character key fragment in a file name. Replaced with the patterns `Openrouter ai*.txt` and `*sk-or-v1*`. The fragment stays in history; it can't be used as a key.
  - The runner writes only into the new run folder and lock files, opens files in exclusive mode, and only reads resumed runs. The key is used only in the HTTP header.
- **Files changed:** `runner/run_experiment.py`, `runner/run_config_benchmark_40task_resume.json`, `runner/tests/test_run_experiment.py` (+5 tests), `.gitignore`.
- **Checks:**
  - 62 of 62 tests pass, three runs in a row.
  - Dry run against a scratchpad copy of the -real folder: 641 to run, starting at #160 attempt 2; 159 skipped; cap basis $2.210105; lock released.
  - Real -real ledger hash unchanged (c5687a1b…, last modified 15:59:23).
- **Unresolved:**
  - The key should still be rotated, because it existed in a plain-text file.
  - The 429-not-billed assumption and the $0.13 estimate should be confirmed on OpenRouter Activity.
  - Model IDs have not been checked against the live catalog.

### 6. Pre-evaluation orientation (read-only)
- **Done:** Re-read work log, resume config, evaluation docs (`benchmark-runbook.md` §6–8, `evaluation-scope-recommendation.md`), and checked the live resume run.
- **Files changed:** `work_log.md` only.
- **Checks (17:53 IST):** `benchmark-40-2026-10-26-resume-01-real` is running (PID 11992 alive, lock held). Log shows 299/800 finished overall, spent $2.91 of $7.00. Resume ledger: 157 rows, $0.697583.
- **Unresolved:** Benchmark not finished; evaluation gates (build, tests, SAST, quality, human review) not built yet; dataset `test_cases`/`solutions` are empty, so a functional oracle is still needed.

### 7. Evaluation Milestone 1: inventory + gate 1 (integrity, syntax, compile)
- **Plan:** separate `evaluation/` package that only reads run folders → inventory of all 800 combinations → gate 1 → CSV/JSON/summary → sample test → test-suite recovery investigation. Followed as planned.
- **Toolchain decision:** no gcc/clang on PATH; MSVC exists but generated C/C++ uses POSIX headers (unistd, sys/wait, pthread, fcntl, dirent), so MSVC would score platform errors as code defects. Installed `ziglang==0.15.2` (clang 20.1.2, glibc headers, target x86_64-linux-gnu) into git-ignored `evaluation/.toolchain`. zig's `-fsyntax-only` fails on Windows (bogus FileNotFound), so C/C++ compile to an object file in a temp folder (no link, no execution). Java: JDK 21 javac + a parse-only helper.
- **Files added:** `evaluation/evaluate.py`, `evaluation/tools/JavaParseCheck.java`, `evaluation/eval_config_benchmark_40.json`, `evaluation/eval_config_benchmark_40_sample.json` (frozen first run only), `evaluation/tests/test_evaluate.py` (16 tests), `evaluation/README.md`, `evaluation/test_recovery/{TEST_SUITE_RECOVERY.md,test_coverage_40.json,test_usability_40_v2.json}`. `.gitignore`: added `evaluation/.toolchain/`. Runner not modified.
- **Bugs found and fixed while testing:** clang output on Windows uses `\r\n`, and the stray `\r` split CSV rows (43 parsed rows for 40). Now normalized, and a test covers it.
- **Checks:**
  - 16/16 unit tests pass (two runs), compiler tests not skipped. They cover: every inventory status, partial last ledger line, results folder refusal, `compile()` never executing code, Java public-class rename, parse-vs-compile vs missing package, Linux POSIX headers, missing `sqlite3.h` → blocked, Wilson and exact McNemar values, end-to-end byte-identical reruns.
  - Sample (frozen `-real` run, 8 tasks, 160 combinations), run twice: all CSV/JSON byte-identical. Inventory matches log step 1: 152 artifacts, 7 failed, 1 incomplete (#160).
  - Found a truncated C file (Task 0 haiku) whose saved source equals the raw `content` exactly, so the truncation is the model's (`finish_reason=length`, 4096 tokens), not a runner bug.
  - Full snapshot (scratchpad `eval-snapshot-1`, taken after generation stopped, lock absent): 800 rows; 404 artifacts (379 + 25 truncated), 14 failed, 3 incomplete, 379 not attempted. Gate 1: 341 pass, 27 fail_integrity (25 truncated, 5 fence residue, 3 overlap), 2 fail_syntax, 2 fail_compile, 32 blocked_dependency (C/C++ only: curl, ldap, libpq, openssl, sqlite3), 0 toolchain errors.
  - All 6 non-truncation failures were checked by hand. All are true positives: prose before the code (×2), Python-style `import … as` in Java, a missing `errno.h`, Markdown fences (×2). The `unbalanced_delimiters` warning fired only on truncated files.
  - Frozen ledger hash unchanged (c5687a1b…).
- **Generation run status (observed, not changed):** `resume-01-real` stopped itself at 18:12:48, `stop_reason=unknown_cost`, after 3 read timeouts (#280, #444, #458). 418/800 finished overall; this run cost $3.5656 + $0.0585 provisional. Lock released.
- **Test-suite recovery (investigation only; no tests run):** the 4 upstream test repositories are public. Commits are recorded in TEST_SUITE_RECOVERY.md; none has a license. The tests are written per original model and bound to that model's function names; inputs and expected values mostly match across models. Usable-check coverage for our 40 tasks (regex upper bound): Python 21, Java 40, C 25, C++ 24; all four languages: 13 tasks.
- **Unresolved:**
  - Generation must be resumed (needs the user's decision; reconcile the 3 timed-out requests on OpenRouter first).
  - 32 C/C++ artifacts are blocked on missing Linux dev headers.
  - Gate 2 needs a per-artifact adapter and a Linux execution sandbox (no WSL distro, no Docker).
  - The upstream tests have no license.
  - Gate 1 for Python is syntax only.

### 8. benchmark-40 resume-01-real paused at 262/641: diagnosis and resume-02
- **Diagnosis (read-only, from the ledger and log):**
  - Not a crash. At 18:12:41–48, three requests timed out ("The read operation timed out"): #280 (qwen, T44, cpp, attempt 3), #444 (mistral, T78, python, attempt 1), #458 (gemini, T78, cpp, attempt 1). Their cost was unknown, so the runner paused by design, wrote the summary and released the lock.
  - Likely cause: the PC went to sleep after it was locked, which dropped the network.
  - Integrity: 298 rows, all parse. 298 raw files, no orphans. 252 sources match 252 success rows. No stale lock.
  - Spend: $1.48552 recorded this run, plus $0.0585 provisional for the 3 timeouts.
  - `benchmark-40-2026-10-26-resume-01` (17:36) is a dry run done in place: empty ledger, placeholders only. It is excluded from every resume.
- **Changes:**
  - `runner/run_experiment.py`:
    - `_keep_system_awake()` uses `SetThreadExecutionState` to stop Windows idle sleep during real runs only, and resets at the end. This is best effort: lid close or a manual Sleep still suspend.
    - The startup log line now includes provisional spend, matching the cap basis.
  - Added `runner/run_config_benchmark_40task_resume_02.json`. It is identical to the resume config except `run_id` (`benchmark-40-2026-10-26-resume-02-real`), `description`, and `resume_from_runs` (-real and -resume-01-real).
  - `runner/tests/test_run_experiment.py`: +2 tests (resume-02 plan against both real ledgers; keep-awake on real runs only).
- **Bug found and fixed:** the first resume-02 config I wrote had `\b` escapes in its paths, which the test skipped silently. The paths now use forward slashes, and the test fails instead of skipping. The run would have refused to start (missing ledger), so nothing unsafe could have happened.
- **Checks:**
  - 64 of 64 tests pass, nothing skipped.
  - Dry run against scratchpad copies of both real folders: 382 to run (first #280 at attempt 3, #444 at 1, #458 at 1), 418 skipped, 0 left unfinished, cap basis $3.754125.
  - Real ledger hashes unchanged (c5687a1b…, 29337a4c…).
- **Projection:** $1.48552 / 262 = $0.00567 per combination, so 382 more is about $2.17, ending near $5.92 of the $7.00 cap. The margin is about $1.08.
- **Unresolved:**
  - The 3 timed-out requests may have been billed; check them on OpenRouter Activity. They are already counted provisionally.
  - If costs run higher, the cap may stop the run near the end. Raising the cap is the user's decision.

### 9. benchmark-40 resume-02-real paused at 339/382: diagnosis and resume-03
- **Diagnosis (read-only):**
  - The run paused itself at 21:44:13 with `stop_reason=unknown_cost`. One qwen request (#585, T101, python) was dispatched at 21:42:10 and hit the hardcoded 120 s read timeout. Keep-awake was active, so this was not sleep.
  - Integrity: 454 rows, all parse. 454 raw files, no orphans. 299 sources match 299 success rows. No lock.
  - Also in this run: #350 (T68, java, attempt 1) and #355 (T68, c, attempt 3) got persistent 429s (not billed). They remain unfinished.
  - Recorded total $5.70255; cap basis $5.89455 (provisional $0.062).
  - Latency measured from the logs (dispatch to saved, including retries): qwen median 31 s, p95 204 s, max 210 s, 11 over 100 s; mistral max 136 s; the others under 84 s.
  - **All 46 remaining combinations are qwen3-8-flash**, so 120 s timeouts are likely to recur.
- **Changes:**
  - `runner/run_experiment.py`: `call_openrouter` takes a `timeout` argument (default 120 s, unchanged). `_send_request` passes `request_timeout_seconds` from the config. This does not affect what is generated or how many calls are made.
  - Added `runner/run_config_benchmark_40task_resume_03.json`. It differs from resume_02 only in `run_id` (`benchmark-40-2026-10-26-resume-03-real`), `description`, `resume_from_runs` (+resume-02-real) and `request_timeout_seconds: 300`.
  - Tests: +2 (resume-03 plan against all three real ledgers, which also checks the config differs from resume_02 only in those keys; the timeout pass-through and its default).
- **Checks:**
  - 66 of 66 tests pass, twice. Both resume-plan tests ran (none skipped).
  - Dry run against scratchpad copies of all three real folders: 46 to run, first #350 (attempt 1), #355 (attempt 3), #585 (attempt 1); 754 skipped; 0 left unfinished; cap basis $5.89455; timeout 300 recorded in the manifest.
  - Real ledger hashes unchanged (c5687a1b…, 29337a4c…, ec78b73e…). No lock left.
- **Projection:** qwen costs at most $0.0021 per attempt. The worst case is 46 × 3 attempts ≈ $0.29, so the cap basis stays at or below about $6.19 of $7.00. Expected runtime 30–45 min (qwen runs one request at a time).
- **Unresolved:**
  - Check the timed-out requests (#280, #444, #458, #585) on OpenRouter Activity. They are already counted provisionally.
  - qwen may keep returning 429s. Any combination that is still rate-limited stays unfinished and needs another resume.

### 10. Evaluation Milestone 2: functional-test framework (no generated code executed, no API calls)
- **Plan:** fetch pinned upstream tests → oracle catalog → adapter framework + workspaces → verdicts/categories → trusted-fixture tests → sandbox proposal. Followed as planned. Runner untouched by this step; steps 8–9 (another session) were read, not changed.
- **Files added:**
  - `evaluation/upstream_fetch.py`: tests and original implementations at pinned commits into git-ignored `evaluation/.upstream/`, plus a hash manifest. 1,462 files (tests: py 200, java 197, c 143, cpp 122; implementations: 200 per language), 18 of them empty, 5.3 MB.
  - `evaluation/functional.py`: catalog / plan / adapters / workspaces / verdicts / executors.
  - `evaluation/functional/{oracle_catalog.json,oracle_catalog.csv,SANDBOX_PROPOSAL.md}`.
  - `evaluation/tests/test_functional.py` (20 tests) and `evaluation/tests/fixtures/functional/*` (trusted mock oracles and implementations).
  - README Milestone 2 section; TEST_SUITE_RECOVERY.md update.
- **Files changed:** `.gitignore` (+`evaluation/.upstream/`), `evaluation/eval_config_benchmark_40.json` (+resume-02-real in `runs`).
- **Task mapping check:** the upstream prompt descriptions equal our dataset's for 40/40 tasks (Task 3 has an empty title upstream).
- **Findings:**
  - Many upstream "tests" are unusable. Some embed their own copy of the function under test (C 62, Python 95, C++ 25 files); for example C Task 44 tests its own fixed-token reimplementation. Some Python "tests" are just copies of the generated code.
  - References (strict: checks + binding to the original + a resolvable symbol): Python 16, Java 33, C 11, C++ 16 task-language pairs; 9 tasks in all four languages (3, 9, 12, 14, 18, 21, 23, 25, 31).
  - Java tests differ per model: 13 of the 33 Java references are the only test with their fingerprint among several.
- **Design:**
  - Adapters are pure aliases. Exact name and arity → auto_approved; normalized name → draft; any binding to `main` → draft; otherwise unresolved.
  - Test files change only at their binding lines. Artifact copies are byte-identical, except that Java copies get the test's `package` line.
  - Reference oracles need human approval in `functional/oracle_review.json`, keyed by test hash.
  - Result categories: passed / failed / error / timeout / not_testable (reason recorded).
  - Executors: `NoSandbox` (default) and `TrustedFixtureExecutor`, which refuses anything not on the fixture hash allowlist.
- **Bugs and fixes during testing:**
  - zig builds the Windows C runtime on first use, so C fixture builds timed out at 20 s. Builds and test runs now have separate limits (`build_timeout` 120 s); the test warms the cache once.
  - `main` was auto-approved by exact name (Java Task 0). It is now draft, with a test.
- **Checks:**
  - 20/20 functional tests and 16/16 Milestone 1 tests pass. Covered: all five categories from real fixture runs (Python passed/failed/error ×2/timeout; C passed/failed/error in assert and PASS/FAIL styles); a reviewed C rename with the artifact's `main`; executor refusal of non-fixture code and of `artifact_origin=generated`; Java facade generation and javac compile (JUnit run not possible: no jar); JUnit XML, unittest and C output verdicts on canned outputs; originals unchanged.
  - Static plan on the Milestone 1 snapshot (scratchpad `functional-plan-2`): 800/800 not_testable (gate1 459, oracle_unreviewed 265, no_oracle 76). Adapter drafts for the 265: auto 148, draft 37, unresolved 80.
  - 185 workspaces checked: artifact copies byte-identical, tests changed only at binding lines, Java tests identical, raw sources unchanged. Plan rerun is byte-identical.
  - Inventory over all three real runs (`--inventory-only`): 754 final (626 generated + 77 truncated + 51 failed), 46 remaining. Matches step 9's dry run.
  - Ledger hashes unchanged (c5687a1b…, 29337a4c…).
- **Sandbox facts (read-only):** WSL 2.6.1 with no distro; virtualization on; Win 11 Home (no Windows Sandbox or Hyper-V); free C: 6.4 GB, F: 6.2 GB, D: 36.5 GB. Recommendation is in SANDBOX_PROPOSAL.md (WSL2 distro on D: + hardened wsl.conf + bubblewrap per test). **Not installed; awaiting approval.**
- **Unresolved:**
  - Sandbox approval.
  - Human review of the 76 proposed oracles (start with the 13 Java single-test ones) and of the 37 draft adapters.
  - The 80 unresolved adapters need manual bindings or signature wrappers. Wrappers are not supported yet by design.
  - The JUnit jar and Python third-party packages are not installed.
  - Generation: 46 combinations remain (resume-03 prepared by the other session).

### 11. Cheap-vs-flagship comparison analysis (read-only; no API calls, no code or runner changes)
- **Benchmark status:** `resume-03-real` finished. 800/800, `combinations_not_finished: 0`. Recorded total $5.835086 = $1.460938 earlier runs + $4.374148 across the four benchmark-40 ledgers. Provisional $0.062 for timeouts (qwen ×2, mistral, gemini). Cap basis $6.030586.
- **Verified billed prices** (from `cost_details` in the raw responses; $ per 1M tokens, input/output):
  - haiku-5.5 0.10/0.50
  - gpt-6-luna 0.10/0.50
  - qwen3.8-flash ≤0.15/0.47 (input is lower with cache)
  - gemini-3.8-flash 0.75/3.75
  - mistral-large-4 1.36/4.18
  - Reference models, used outside the benchmark: claude-sonnet-5.5 2/10, qwen3.8-max-0902 2/6.
- **Per model** (160 combinations each; $ total / per combination / per gate-1 pass; gate-1 pass):
  - haiku: 0.1599 / 0.00100 / 0.00136; 118
  - luna: 0.1026 / 0.00064 / 0.00077; 134
  - qwen: 0.4580 / 0.00286 / 0.00463; 99 (empty responses in 185 of 299 billed attempts)
  - gemini: 1.1980 / 0.00749 / 0.00915; 131
  - mistral: 2.4555 / 0.01535 / 0.02013; 122
- **By stratum (gate-1 pass):**
  - Baseline F–G: 231/240 (96%).
  - Security A–E: 373/560 (67%). All integrity failures, blocked dependencies and most missing artifacts are in A–E.
  - The 9 tasks with oracles in all four languages are all F/G.
- **Conclusions:**
  - No benchmark model is a flagship by billed price; all are cheaper than the recorded sonnet and qwen-max prices. A price-tier comparison is defensible: low (haiku, luna, qwen) vs higher (gemini, mistral).
  - A 9-task functional subset would be biased toward non-security tasks.
  - No functional or SAST results exist yet, so only structural validity, reliability and cost can be compared.
- **Files changed:** `work_log.md` only.

### 11. Sandbox installed and verified; final gate 1; oracle/adapter review reports (no generated code executed)
- **Plan:** install the approved sandbox (proposal steps 1–3) → harden → security battery (stop if unreliable) → WSL executor tested on fixtures only → final gate 1 (generation had finished) → review reports. Followed as planned. Runner untouched; the resume-03 run was active at the start and was not disturbed (it finished on its own at 23:14).
- **Installed** (exact versions in `evaluation/sandbox/SANDBOX_STATE.md`):
  - WSL distro `eval-sandbox` (Ubuntu 24.04.5) on `D:\wsl\eval-sandbox`.
  - gcc/g++ 13.3.0, python 3.12.3, openjdk-21-jdk-headless 21.0.12.1, bubblewrap 0.9.0, util-linux 2.39.3, coreutils 9.4.
  - JUnit console 1.11.4 (SHA-1 matched Maven Central; SHA-256 b016ef6b…).
  - Not installed: dev headers, Python packages, `.wslconfig` changes.
- **Hardening problems found and fixed:**
  - (a) `[interop] enabled=false` left the `WSLInterop-late` binfmt handler registered: a Windows exe was passed to `/init`, which tried the host over vsock (timed out; exit 0 masked the failure). A boot command now unregisters both handlers; the exe now fails with `Exec format error`.
  - (b) `/usr/lib/wsl/drivers` is a read-only 9p view of the Windows driver store that WSL always mounts. It is hidden in the sandbox (tmpfs over `/usr/lib/wsl`).
  - (c) The first hardening check tested `/mnt` for emptiness rather than for mounts; it was fixed and the leftover empty folders removed.
- **Sandbox problems found and fixed:**
  - (a) RLIMIT_NPROC set before bwrap made the kernel refuse the user namespace (EAGAIN). The process limit is now applied inside the sandbox.
  - (b) **False passes:** while bwrap failed to start, 4 checks reported PASS. Every check now requires a probe-ran marker.
  - (c) The sandbox root and the workspace were writable and unbounded (disk fill possible). Now the root is read-only, the input is read-only `/src`, `/ws` is a 256 MB tmpfs and `/tmp` a 64 MB tmpfs, and output is capped by the file-size limit.
  - (d) vsock is not network-namespaced, so seccomp now blocks AF_VSOCK (generated BPF, no new package).
  - (e) The network probe crashed on a missing `/sys`; it now uses `if_nameindex`.
  - (f) The file-size check accepted any error; it now requires EFBIG.
  - (g) Inline `wsl.exe bash -c` scripts were mangled; a fixed root-owned `receive_and_run.sh` (numeric arguments only) replaces them.
- **Checks:**
  - check_hardening 10/10.
  - check_bwrap 22/22 on every run of the final script.
  - Negative control (unsafe runner): every isolation check fails. The only passes are 2 timeout checks, Windows-exe, and 2 toolchain controls, so the battery detects missing isolation.
  - Tests: test_wsl_sandbox 9/9 (fixture outcomes in the sandbox for Python ×5, C ×5 and Java/JUnit ×2; refusals for missing approvals ×4, tamper, unlisted fixture, unverified sandbox), test_functional 22/22, test_evaluate 16/16.
- **Policy change:** adapters are never auto-approved. Labels are `proposed_exact` (name + arity + declared types), `proposed_loose`, or `unresolved`. Only a human `approved` with a reviewer is usable. Added `normalize_type`/`signature_match`, and a test where an exact name with different types is only loose.
- **Final gate 1** (`evaluation/results/benchmark-40-final-gate1`, four real runs, generation finished at 800/800):
  - 734 artifacts (654 + 80 truncated), 66 failed.
  - Gate 1: 604 pass, 85 fail_integrity, 2 fail_syntax, 2 fail_compile, 41 blocked_dependency.
  - No partial lines, duplicates, orphans or unplanned rows.
  - Ledger hashes: c5687a1b, 29337a4c, ec78b73e, 00536e67.
- **Final functional plan** (`…-final-functional-plan`, `--execute`): sandbox verified; 800/800 not_testable (oracle_unreviewed 336, no_oracle 268, gate1 196). No workspace was built, so **no generated code ran**. Adapter proposals: 135 exact, 80 loose, 121 unresolved.
- **Review reports** (`evaluation/functional_review.py`): `functional/review/ORACLE_REVIEW.md` (76 oracles, Java first, single-test ones first) and `ADAPTER_REVIEW.md` (336), plus JSON templates with every decision null. The MDs are git-ignored (they quote unlicensed upstream tests). Spec overlap of 0 literals: Java 20/33, Python 7/16, C 2/11, C++ 7/16. Example: Task 44 Java asserts uppercase plus an alphanumeric whitelist that the prompt never states.
- **Unresolved:**
  - Human review of the 76 oracles and 336 adapters (nothing is approved).
  - Result integrity is not protected against an adversarial artifact.
  - gcc (sandbox) versus clang (gate 1) build differences.
  - Optional packages are still not installed.

### 12. Final evaluation run: review, functional tests, static scans, consolidated results
- **Plan:** freeze snapshot → install scanners (background) → oracle review → adapter review → functional tests in the sandbox → scans → consolidation. Followed. No API calls; runner and raw outputs untouched (ledger hashes c5687a1b, 29337a4c, ec78b73e, 00536e67; all 734 artifact hashes re-verified at the end).
- **Snapshot:** `evaluation/results/SNAPSHOT_benchmark-40-final.json`, id 30d64f1cb11b3160…: 800 planned, 734 artifacts (654 + 80 truncated), 66 failed generations, 0 hash mismatches.
- **Oracle review** (AI-assisted on the user's instruction; reviewer string says so; needs human confirmation): 36 approved, 40 rejected (`functional/oracle_review.json`, with reasons).
  - Rejections: unspecified behaviour; services/network needed; out-of-spec cases; a wrong-task test (25/python tests validate_pin); a self-inconsistent test (21/c).
  - **Wrong expected values** in 23/java and 23/python, e.g. findNb(1000)=−1, not 10. 23/c and 23/cpp are correct; the upstream C++ file even carries "todo java and python" comments.
- **Adapter review:** 174 reviewed, 141 approved initially. Rejections are API mismatches only (List vs int[], arity, pointer types, Solution:: class, in-place sorting).
- **Functional run v1** (`benchmark-40-final-functional`, now marked SUPERSEDED): 125 passed, 16 errors. All 16 were build failures and none was a model fault:
  - 9/c: the test's own prototype conflicts with the artifact's signature;
  - 14/cpp: the test needs a Solution class (missed by the catalog);
  - 25/java: **my facade lacked java.util imports**;
  - #527: the target method is private.
  - **Verdict bug:** a failed build was reported as "crash" / "no JUnit report".
  - Fixes: facade imports; `pad_skipped_steps` (failed build → error: build_failed); 11 adapters rejected with post-execution notes (130 approved).
  - Tests: +2 (host and sandbox build_failed); test_functional 23/23, test_wsl_sandbox 9/9, test_evaluate 16/16.
- **Functional run v2** (`benchmark-40-final-functional-v2`): self-test 31/31 (my earlier "32" was a miscount; the hardening script has 9 checks; SANDBOX_STATE.md corrected). 130 executed: **130 passed**, 0 failed, 0 error, 0 timeout. Every run shows a real test execution (unittest "Ran N", JUnit XML, C/C++ run step). Ceiling effect: no model separation.
- **Scanners installed** (sandbox venv `/opt/scanners`, no code execution): semgrep 1.182.1, bandit 1.9.4, flawfinder 2.0.20, lizard 1.24.3; cppcheck 2.13.0 (apt). Semgrep packs pinned: cwe-top-25 725e296a…, security-audit 83d7cde9… (`evaluation/scanners/rules`, git-ignored, manifest committed).
- **Scans** (`benchmark-40-final-scans`, `evaluation/scan.py` + `sandbox/run_scanners.sh`): 734/734 files scanned by semgrep, 0 skipped rules; 1,600 findings (flawfinder 1,284, cppcheck 192, semgrep 73, bandit 51). All 39 semgrep parse errors are on gate-1 failures.
- **Consolidation** (`benchmark-40-final-consolidated`, `evaluation/consolidate.py`): 800-row combined CSV/JSON with separate evidence columns; by-model and by-model×language tables; RESULTS.md.
  - Headline security metric changed to ≥1 medium/high finding, because flawfinder level-0 printf noise dominates "any".
  - Fixed: "scanned" denominator (was tied to lizard NLOC>0) is now all gate-1 passes.
- **Key results:**
  - Gate 1 pass: gpt-6-luna 83.8%, gemini 81.9%, mistral 76.2%, haiku 73.8%, qwen 61.9%.
  - C is weakest for every model.
  - Medium/high potential findings: 12–18% of gate-1-passing artifacts per model, driven by C (41–60%).
  - Cost: gpt-6-luna $0.103 and haiku $0.160 versus mistral $2.456; it sums to the $4.374 benchmark-40 ledger total.
- **Unresolved:**
  - Human confirmation of the AI-assisted oracle and adapter decisions.
  - Functional coverage is 130/800 with a ceiling effect.
  - Scanner findings are not triaged.
  - No CodeQL/SonarQube.
  - 41 dependency-blocked and 66 failed generations untested.

### 13. Paper rewritten as the empirical study; compiled
- **Plan:** audit the draft → verify references → triage proposals → generate numbers/tables/figures from results → rewrite → compile → consistency checks. Followed. Raw outputs, evaluation results and review files unchanged (ledger hashes c5687a1b, 29337a4c, ec78b73e, 00536e67; no file in evaluation/results newer than 00:14). `Latex_paper.txt` unchanged.
- **Audit** (`paper/PAPER_AUDIT.md`):
  - The draft presented ContractGuard (an unimplemented framework) as the contribution, with RQ2–RQ6, baselines and benchmarks that were never run. Its setup also claimed 50 tasks, temperature 0.9 with the base study's wording, SonarQube/CodeQL, and seeds/bootstrap/Holm. All were removed or corrected.
  - Six related-work numbers differed from the abstracts (SusVibes, Tessa, BaxBench, Dai, CSDD, METR) and were corrected.
  - 16 unverified references used only for the framework were dropped.
- **References:** 14 arXiv entries verified via the arXiv API (`paper/refcheck/`); the base study and SecureCoder were verified against the local PDFs. All 24 references are cited; no undefined citations.
- **Triage proposals** (`paper/data/triage_ai_proposed.csv`, separate from the evaluation results): 65 findings labelled from source context. 9 likely TP, 17 need context, 39 likely FP. Every row is marked "AI-proposed; NOT human-confirmed".
- **Assets:** `paper/scripts/make_assets.py` generates 112 number macros, 12 tables and 4 figures from the results. The categorical palette was validated with the dataviz validator; figures carry hatching and direct labels plus matching tables. PNG previews were inspected.
- **Paper:** `paper/main.tex` → `paper/build/main.pdf`, IEEE conference format, 7 pages, 0 errors, 0 overfull boxes, 0 undefined references. `paper/scripts/check_paper.py`: cites/bib and refs/labels consistent; every literal number reviewed.
- **Toolchain:** tectonic download denied by the permission policy (external binary), so it was not used. Installed instead from Ubuntu repositories in eval-sandbox: texlive-latex-base/latex-extra/publishers 2023.20240207-1 (pdfTeX 1.40.25) and poppler-utils 24.02.0. Build: `bash paper/build.sh`.
- **Fixes made while checking:**
  - "nearly all algorithmic" executed tasks corrected (3 of 14 are from security strata);
  - windows.h is not "third-party";
  - a garbled Flawfinder sentence;
  - cost units;
  - two overfull tables;
  - the METR claim made precise after reading the full abstract.
- **Unresolved, decisions for the authors:**
  - title (registered title describes the unimplemented framework);
  - human confirmation of oracle/adapter/triage decisions;
  - author block, affiliation, e-mails, acknowledgment, repository URL;
  - venue;
  - whether to keep the ContractGuard future-work paragraph.

### 14. Methodology audit vs. the base paper (read-only)
- **Done:** audited tools, configurations, coverage, metrics and comparability from scripts, manifests and raw outputs; base-paper methodology read from its PDF (Sec. III-C/E). No API calls; no results, paper or raw outputs changed. Written to `evaluation/METHODOLOGY_AUDIT.md`.
- **Key evidence:**
  - Semgrep scanned 734/734 files (`raw/semgrep.json` `paths.scanned`); Bandit analysed 187/187; Flawfinder and Cppcheck ran on the C/C++ inputs (outputs plus versions; per-file coverage not logged).
  - Lizard: 3,084 functions in 714/734 artifacts.
  - Functional v2 self-test 31/31.
  - **CodeQL/SonarQube: not installed and not executed on our benchmark.** The repo only contains the base paper's scripts and its reports for its own models.
- **Defects found (not fixed, per instruction):**
  - `consolidate.py:148` `cppcheck_per_kloc` divides by all-language NLOC, so C/C++ density is understated about 2.2× (Mistral 9.66 → 21.27). Affects paper Table XI.
  - `table_by_model_language.csv` lacks mean_nloc and functions_ccn_gt10 (they are in `summary.json`).
- **Unreported evidence:** gate-1 paired McNemar tests (160/160 cells); 7/10 pairs significant after a manual Holm correction.
- **Recommended additions (smallest first):**
  1. fix the Cppcheck denominator;
  2. report the gate-1 paired tests;
  3. run identical tools on the base paper's outputs for the same 40 tasks (local, no API);
  4. human-confirm the 65 triage labels;
  5. CodeQL (needs a download and permission);
  6. CCD plus a per-language quality table.

### 15. Evaluation improvements 1, 2, 3, 6; paper regenerated; base-comparison assessment
- **Plan:** versioned consolidation (fix + new metrics) → independent verification → regenerate paper assets and verify every number → assess base-paper comparability without running it. Followed. No API calls; nothing installed; no raw artifacts or earlier results changed (ledger hashes c5687a1b, 29337a4c, ec78b73e, 00536e67; 734 artifact hashes unchanged; `benchmark-40-final-consolidated/` (v1) untouched; base repo clean).
- **Code changes:**
  - `evaluation/consolidate.py` → CONSOLIDATE_VERSION 2:
    - `cppcheck_per_kloc_c_cpp` replaces `cppcheck_per_kloc` (C/C++ NLOC denominator);
    - per-artifact `sum_ccn` and `ccd` from raw Lizard, cross-checked against the summary NLOC;
    - mean/pooled CCD;
    - `quality_by_model_language.csv`;
    - `table_by_model_language.csv` gains mean_nloc, functions_ccn_gt10 and CCD;
    - `paired_gate1.csv` and `summary.json` `paired_gate1`: exact McNemar at full precision, Holm, Newcombe method-10 paired difference, conditional OR with exact CI;
    - `METRICS.md` (definitions and unavailable metrics).
  - New `evaluation/verify_consolidated_v2.py` (independent recomputation from raw Lizard/Cppcheck/gate-1 files; McNemar and Clopper-Pearson checked with SciPy 1.17.1; independent Holm).
- **Outputs:** `evaluation/results/benchmark-40-final-consolidated-v2/`.
  - Verification: ALL CHECKS PASS (30 quality cells, 10 pairs).
  - v1 vs v2: 0 differences in shared summary keys and in 800 rows × shared columns; only new fields added.
- **Results:**
  - Corrected Cppcheck/KLOC (C/C++): Haiku 5.21, Luna 3.69, Gemini 5.56, Mistral 21.27, Qwen 7.68, all 7.76 (v1 understated ~2.2×).
  - Mean CCD overall 0.326.
  - Paired gate 1: 7/10 pairs significant after Holm. Luna vs Gemini p_Holm=1.00; Haiku vs Mistral 1.00; Gemini vs Mistral raw 0.049, Holm 0.147; Luna vs Mistral Holm 0.047.
- **Paper:**
  - `paper/scripts/make_assets.py` reads v2; new tables `tab_paired_gate1.tex` and `tab_quality_lang.tex`; CCD added.
  - Cost per 1,000 gate-1 passes is now computed unrounded (fixed 1.35→1.36 Haiku, 9.14→9.15 Gemini).
  - `main.tex`: Statistics and complexity-metric definitions, RQ1 paired results plus table, RQ4 rewrite, reliability note on the v1 correction; refs holm1979 and newcombe1998 added.
  - Build: 8 pages, 0 errors, 0 overfull, 0 undefined; cites/refs consistent.
  - New `paper/scripts/verify_paper_numbers.py`: 181 checks, 0 failed, 125/125 macros verified against row-level sources (not summary.json), plus table rows and 7 prose claims.
- **Base comparison (assessment only, not run):** `evaluation/base_compare/` (`classify_base_versions.py` → `base_version_classes.csv`, `count_escape_artifacts.py`, `BASE_COMPARISON_PROTOCOL.md`).
  - The base CodeQL/SonarCloud reports analysed test-repository copies.
  - Of 600 published C/C++/Python files, 439 are identical to the test copies; the others are harness edits (96), escape fixes (4) and other edits (59), including API and logic changes.
  - The 73 manual fixes are not individually identifiable (no ledger).
  - 37 published C/C++ files have doubled-backslash escapes.
  - Java has no published copy.
  - Protocol proposed: published C/C++/Python, identical pipeline, cohort-level comparison, pre-declared sensitivity analyses.
- **Unresolved:**
  - Protocol approval.
  - Java handling.
  - Paired tests treat the 160 cells as independent (the 4 language cells of a task are correlated).
  - Unavailable metrics: cognitive complexity, SonarQube categories, Python/Java linting, file-level LOC, coverage.
  - AI-proposed triage, oracle and adapter decisions still not human-confirmed.

### 16. Secondary comparison with the base study's published outputs (C/C++/Python)
- **Feasibility:** the main paper was already complete, so the comparison could not delay it. The full pipeline took 3m47s. Nothing installed, no API calls, no CodeQL/SonarQube.
- **Code (new, `evaluation/base_compare/`):**
  - `run_base_pipeline.py`: copies the 600 published files byte-for-byte (read-only) and writes a snapshot with hashes. Runs evaluate.py's own gate-1 functions with the benchmark toolchain config, then `scan.main`, and re-hashes the originals.
  - `compare_cohorts.py`: computes the comparison.
  - `verify_base_comparison.py`: independent recomputation from raw gate-1 JSON, findings.csv, raw Lizard and Cppcheck XML.
  - `write_comparison_md.py`: writes the supplementary RESULTS.md.
- **Outputs:**
  - `evaluation/results/base-40-published-v1/` (snapshot 1736c9c488b2, gate1/, scans/, plus run log `base-40-published-v1.log`).
  - `evaluation/results/base-40-comparison-v1/` (cohort_rows.csv, cohort_comparison.csv, paired_cells.csv, comparison.json, RESULTS.md).
- **Integrity:**
  - Published originals: 0 of 600 changed.
  - Our artifacts: 0 of 734 changed.
  - No earlier result file was modified.
  - Scanner versions, rule hashes and config are identical to the main scan.
  - 48/48 unit tests pass.
- **Base gate 1:** 482 pass, 93 blocked, 23 compile, 2 syntax.
- **Scans:** 1,358 findings, 0 unmapped, 5 semgrep partial-parse errors.
- **Results (ours − base, pp, Newcombe 95% CI):**
  - Gate 1 per planned cell: −8.0 [−12.8, −3.2]. Wilcoxon signed-rank on 120 cells, p = 2.1e−4 (base higher in 39 cells, ours in 13). The gap comes from our 55 failed generations and 68 integrity failures (65 truncated).
  - Gate 1 per artifact: −0.7 [−5.4, 3.9].
  - Sensitivity analyses:
    - S1 (identical files only): −8.7 [−13.2, −4.1];
    - S2 (no escape-pattern files): −1.7 [−6.4, 2.9];
    - S3 (blocked excluded): −9.0 [−12.6, −5.4];
    - S4 (post-hoc, truncation flag ignored; +2 passes): −0.3 [−5.0, 4.3].
    
    So the per-artifact comparison has no robust direction.
  - ≥1 medium/high potential finding: −3.0 [−8.3, 2.3] overall; Python −7.2 [−13.2, −1.3].
  - Base files are much shorter: mean function NLOC 14.3 vs 44.5. This confounds per-file and per-KLOC rates (Cppcheck/KLOC base 25.4 vs ours 7.8).
- **Fixes made along the way:**
  - The verifier caught double rounding in the paper generator and in RESULTS.md (values formatted from 4-dp CSV); both now use unrounded JSON.
  - A LaTeX build failure went unnoticed because a stale PDF remained; `paper/build.sh` now deletes the old PDF and exits 1 on any LaTeX error.
- **Paper:**
  - New subsection "Secondary Analysis: Base-Study Outputs" with `tab_base_cohort.tex` (all numbers from 27 new macros), plus one sentence in External validity.
  - Base temperature 0.9 was verified in the base repo's `*_integration.py`.
  - verify_paper_numbers: 218 checks, 0 failed, 152/152 macros.
  - Build: 9 pages (was 8), 0 errors, 0 overfull, 0 undefined.
- **Unresolved:** page count vs venue limit; human confirmation of AI-assisted decisions (unchanged); Java not compared.

### 17. Pilot-10 evaluated as a separate supplementary check (user decision: not pooled)
- **Check:** the 10 pilot tasks (24, 37, 38, 39, 41, 42, 43, 46, 51, 104; all from strata A–D) are disjoint from benchmark-40, and the paper already says pilot tasks were excluded.
  - Models and generation settings are identical, and so is the prompt (`build_instruction` hash and system prompt are the same in commits 901ce49 and 42d17b0 and in the working copy).
  - Pilot runs were used to validate the runner, so they are reported separately.
- **Code:**
  - `evaluation/eval_config_pilot_10.json` reads the two `-real` pilot runs.
  - `evaluation/pilot_check.py` freezes a snapshot with artifact and ledger hashes, runs the unchanged `scan.py`, summarizes with the consolidation-v2 definitions, and cross-checks per-artifact values against raw findings and Lizard output.
- **Outputs:**
  - `results/pilot-10-gate1-v1/`
  - `results/SNAPSHOT_pilot-10.json` (184 artifacts)
  - `results/pilot-10-scans-v1/` (558 findings, 0 unmapped; versions, rule hashes and config identical to the main scan)
  - `results/pilot-10-summary-v1/` (`pilot_rows.csv`, `summary.json`, `RESULTS.md`; 0 cross-check mismatches)
- **Results:**
  - Generation: 200 planned, 184 artifacts (29 truncated), 15 failed, 1 incomplete (the Qwen IncompleteRead combination; it counts as not passing).
  - Gate 1: 142/200 = 71.0% (77.2% per artifact). By language: Python 92.0%, Java 78.0%, C++ 58.0%, C 56.0%.
  - ≥1 medium/high finding: 25.4% overall, C 75.0%. Higher than benchmark-40 as expected, since the pilot has only security tasks.
  - Same language pattern as the main study. Per-model differences are not resolvable (n = 40 per model).
- **Paper:** one sentence in External validity (8 new macros).
  - verify_paper_numbers: 229 checks, 0 failed, 161/161 macros.
  - Build: 9 pages, 0 overfull, 0 undefined.
- **Integrity:** benchmark artifacts 0/734 changed; pilot ledgers verified unchanged. No API calls.

### 18. Pilot-10 models re-verified from the data (user query: "pilot ran on different models")
- **Finding:** pilot-10 used the same five models as benchmark-40.
  - Provider-returned `response_model_id` in all pilot responses: claude-haiku-5.5, gpt-6-luna, gemini-3.8-flash, mistral-large-4-0, qwen3.8-flash (1 Qwen response has none).
  - The run manifests record the same model IDs, temperature 0.2, top_p 0.95, max_tokens 4096, 3 attempts, `dry_run: False`.
- **Runs with other models:**
  - The 20-task pilots of 23–25 Oct (Sonnet 5.5 / Qwen3.8 Max; one plan with GPT-4o, Gemini Flash 1.5, Codestral, Llama 3.3) and `benchmark-90/pilot-90task` are dry runs with placeholder files only.
  - The task-7 one-task runs (Sonnet 5.5, Qwen3.8 Max, etc.) are connection tests: 4 real Python files.
  - `output/CODESAVE@ALL` = 734 benchmark + 184 pilot-10 + 4 task-7 files, all byte-identical to the run outputs.
- **Results unchanged.** The paper sentence now says "the same five models"; verify 229/0, 161/161 macros; build 9 pages, 0 overfull, 0 undefined.
- **Document:** model evidence and an "earlier runs not evaluated" table added to the Pilot-10 section.

### 19. Review and correction of the full paper (Generating_Secure_And_Quality_Code.pdf), 2026-10-27
- **Decision (user):** option (a). This paper = updated measurement (RQ1, reported) + framework design with an evaluation plan (RQ2-RQ6, future tense). Implementation results go in a follow-up paper.
- **Number check:** every RQ1 number in the PDF matched `evaluation/results/*` and `paper/generated/*` (gate 1 per model incl. integrity 31/19/14/11/10, McNemar/Holm pairs, findings, CWEs, triage, quality, cost, base comparison, pilot). The PDF's 1-dp Wilson CIs (e.g. 72.4-78.4, Luna 77.3, Qwen 54.2) are correct when computed from counts.
- **Pipeline bug found (not fixed):** `evaluation/evaluate.py:643` `wilson()` rounds to 4 dp; `paper/scripts/make_assets.py` then formats to 1 dp, so some endpoints in `paper/main.tex` are 0.1 low (78.3 vs 78.4, 77.2 vs 77.3, 54.1 vs 54.2). Raw results unchanged.
- **New files (originals untouched):** `paper_proposal/main.tex`, `paper_proposal/Generating_Secure_And_Quality_Code_v2.pdf` (12 pages, 0 errors, 0 overfull, 0 undefined). Source: Latex_paper.txt version 3 + newer content from the PDF.
- **Changes:** measurement moved to Section III and leads; framework and evaluation plan in future tense ("Planned Implementation", "Evaluation Plan"); abstract, contributions, RQ intro, conclusion rewritten for this framing; Table I last row marked "proposed design"; threats split into measurement vs planned evaluation; CCD column added to Table IV; cost-waste sentence (31% of $4.37 on empty/invalid attempts); models described as "lower-cost" with the price spread stated (was contradictory).
- **Corrected claims:** "replication" -> "updated, partial re-measurement"; "undocumented manual fixes in 161 files" -> "161 files differ from the test copies for undocumented reasons, possibly including manual fixes"; functional coverage attributed to base tests and our approval criteria; "because of our failed generations" -> "largely reflecting"; "no robust direction" in sensitivity analyses was wrong (S1 -8.7 [-13.2,-4.1] and S3 -9.0 [-12.6,-5.4] favour base files; S2 -1.7, S4 -0.3 near zero) -> rewritten; "neither price nor model choice" limited to security findings.
- **Still open (authors):** affiliation/emails, empty Acknowledgment, 9 incomplete references (TODO comments in the .bib section), confirmation of AI-proposed oracle/adapter/triage labels, venue page limit (now 11 pages + a short 12th).
- **Build note:** eval-sandbox TeX lacks algorithm.sty/algorithmic.sty; main.tex falls back to minimal definitions only when they are missing (Overleaf uses the real packages).

### 20. Project corpus document and artifact (read-only on all project data)
- **Done:** wrote `PROJECT_CORPUS.md` (21 sections, about 10,000 words, 23 tables). It is a detailed, searchable reference covering:
  - the base paper and SecureCoder (full method and numbers);
  - the framing history; the dataset; task selection; models and billed prices;
  - prompt and settings; the runner design; every run, incident and cost;
  - the evaluation pipeline (modules, tools and versions, sandbox, gate 1, functional, scanners, statistics);
  - all benchmark-40 results; the base comparison; pilot-10;
  - both papers (RQs, framework components G1–G9, baselines B0–B8, metrics); defects and open items;
  - a glossary, quick answers, a file map and the commit history.
  - Each section has a "Search terms" line of synonyms. Facts are tagged [F], recommendations [R], assumptions [A].
- **Artifact:** rendered from the same Markdown (scratchpad `build_corpus_page.py` + `corpus_template.html`), with search and highlighting, a section index and clickable term chips. Published at https://claude.ai/artifact/5FgGJs1Ndy6f4p7Rfq72Ra.
- **Checks:** HTML nesting checked; script syntax checked with `node --check`.
  - Numbers were copied from the primary result files (RESULTS.md v2, gate1 summary.json, base-40-comparison and pilot-10 RESULTS.md, ledgers and raw responses, both PDFs, `paper/` and `paper_proposal/` `main.tex`).
  - Derived figures were cross-checked against published values: language totals sum to 604; 96/604 = 15.9% and 130/800 = 16.2% match the proposal abstract; resume-03 cost 0.132536 = 4.374148 minus the other three runs.
- **Files changed:** `PROJECT_CORPUS.md` (new) and `work_log.md`. No project data, results or code changed. No API calls.
- **Not verified:** the mapping of the `Papers.txt` IEEE links to titles. The early-draft related-work numbers in §15.3 are flagged as superseded by `paper/main.tex`.
