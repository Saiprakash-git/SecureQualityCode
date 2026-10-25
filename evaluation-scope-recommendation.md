# Reduced Evaluation Scope and Validation Plan

## Direct answer

For a major-project study, use:

- **20 tasks** for a paid pilot and cost estimate;
- **48 tasks** for the minimum defensible prototype comparison;
- **60 tasks** for the recommended final reduced benchmark;
- **80-100 tasks** if the paper must make stronger model-ranking or language-generalization claims;
- all **200 tasks** only for a close replication of the base paper, not as the minimum needed for this project.

The recommended target is **60 paired tasks**, run with the same task set for every model and language. With four languages and four models, this produces:

```text
60 tasks x 4 languages x 4 models = 960 generated artifacts
```

That is much smaller than the original 4,000-artifact experiment, while still covering the major defect families and leaving enough observations for per-model and per-language summaries.

## Why not choose only 10 or 20 tasks?

Ten or twenty tasks are useful for engineering and cost estimation, but they are too small for a reliable final comparison. A rough worst-case 95% proportion estimate has a margin of error of approximately:

- 20 tasks: plus or minus 22 percentage points;
- 48 tasks: plus or minus 14 percentage points;
- 60 tasks: plus or minus 13 percentage points;
- 96 tasks: plus or minus 10 percentage points.

These are approximate independent-observation bounds. Because the same task is paired across models, the paired design is more efficient for comparing models, but it does not make a 20-task study representative of 82 CWE labels. Report confidence intervals and avoid overclaiming from a small sample.

## What the repository actually contains

The checked-out dataset contains 200 unique prompt JSON files:

- 100 under `promptDescription-0XX`;
- 100 under `promptDescription-1XX`;
- 140 tasks are security-relevant under a broad tag/title/description classification;
- 60 are general algorithm or utility tasks;
- 82 distinct CWE labels occur, but many appear only once;
- the prompt JSON files do not contain usable test cases or populated reference solutions for all four languages.

The original paper's 4,000 files came from 200 tasks multiplied by five models and four languages. Do not describe the local 200 prompts as 1,000 independent tasks.

## Recommended 60-task composition

Select tasks by strata, then use a seeded random choice within each stratum. The exact IDs must be recorded in the run manifest.

| Stratum | Number | Purpose |
|---|---:|---|
| Memory safety and low-level correctness | 8 | C/C++ buffer, pointer, allocation, overflow, and bounds behavior |
| Input, injection, and file/path handling | 10 | CWE-20, command/SQL/LDAP/XPath injection, path traversal, file inclusion, upload |
| Authentication, authorization, sessions, and secrets | 8 | Authentication, privilege, session, password, sensitive-data, and access-control behavior |
| Cryptography and randomness | 6 | Weak algorithms, key/IV handling, password hashing, predictable randomness |
| Concurrency, networking, and resource handling | 6 | Locking, races, sockets, resource exhaustion, denial-of-service patterns |
| General algorithms and data structures | 10 | Correctness baseline with low security intent |
| General quality and maintainability | 6 | Parsing, logging, error handling, complexity, API use, and maintainability |
| **Total** | **60** | |

Do not try to represent every one of the 82 CWE labels in 60 tasks. Instead, cover the high-frequency categories and ensure that rare but important families appear at least once where possible. Report the exact CWE coverage and mark single-example categories as exploratory.

## Minimum 48-task version

If cost is tighter, use 48 tasks:

- 30 security tasks;
- 8 general correctness tasks;
- 6 quality/maintainability tasks;
- 4 concurrency/network/resource tasks.

Use this only for a prototype or exploratory paper. It is sufficient to compare a complete verification loop against raw generation on paired tasks, but not sufficient for confident conclusions across every language and CWE family.

## Pilot design

Use 20 tasks before committing to the final model matrix:

- 4 memory/input-security tasks;
- 4 authentication/crypto tasks;
- 3 concurrency/network tasks;
- 5 algorithm/correctness tasks;
- 4 quality/error-handling tasks.

Run three or four candidate models and one generation per task/language/model. The pilot should answer:

1. Which model IDs are affordable and reliable?
2. What is the average input/output token count?
3. How often do outputs fail parsing or compilation?
4. How much time does each analyzer take?
5. Which task categories produce useful findings?
6. How much manual review effort is required?

Do not use pilot results as the final model ranking unless the pilot was pre-registered as part of the study. Use it for cost and infrastructure planning.

## What should be automated?

Automate the repeatable gates for every artifact:

1. Response capture, token usage, model ID, parameters, latency, and cost.
2. Code extraction and language/output completeness checks.
3. Syntax parsing, compilation, imports, and type checking.
4. Unit and integration tests.
5. Static security analysis such as CodeQL, SonarQube, Semgrep, Bandit, or language-specific equivalents.
6. Dependency, secret, and configuration scanning.
7. Lint, complexity, duplication, reliability, and maintainability analysis.
8. Sandbox execution with timeout, memory, filesystem, process, and network restrictions.
9. Finding normalization, deduplication, CWE mapping, severity aggregation, and metric calculation.
10. Pre/post comparison for repair attempts.

Automation gives consistency and scale, but a tool finding is not automatically a confirmed vulnerability.

## What still requires manual review?

Review a stratified sample rather than manually reading every generated file:

- all critical/high findings from the selected analyzers;
- all analyzer-disagreement cases;
- random medium/low findings by language and model;
- a clean-code sample to estimate false negatives and tool blind spots;
- every accepted repair in the final reported repair set;
- semantic/business-logic cases that lack reliable automated oracles.

For a 60-task final benchmark, review approximately 15-25% of artifacts or a minimum of 100-150 findings, whichever is smaller and feasible. Two reviewers should independently label sampled findings as true positive, false positive, duplicate, not enough context, or confirmed clean. Use a third reviewer or consensus process for disagreements.

## Semantic correctness without supplied tests

The local prompt JSON is not enough to validate semantics because its `test_cases` and `solutions` fields are empty in the checked-out tasks. Build an evaluation oracle before generation:

1. Recover or recreate the generated-code test repositories linked by the dataset README.
2. Verify that each selected task has a test suite for each selected language.
3. If a test suite is unavailable, create language-specific tests from the prompt specification before evaluating models.
4. Keep tests out of the generation prompt.
5. Add edge cases and negative cases, not only examples from the prompt.
6. For selected security tasks, add adversarial inputs that exercise the weakness.
7. Mark tasks without a reliable oracle as `manual_semantics_only` and exclude them from aggregate functional pass-rate claims.

Reference solutions can help reviewers understand intended behavior, but they are not a complete oracle. Tests, properties, differential comparison, and manual review should be combined.

## Evaluation funnel

```text
Generated response
  -> complete/parseable output?
  -> syntax/build/type checks
  -> reference and edge-case tests
  -> property/fuzz/negative tests where applicable
  -> SAST and dependency/secret scans
  -> quality and maintainability analyzers
  -> human validation sample
  -> repair experiment
  -> re-run all gates
  -> accepted, rejected, or human-review outcome
```

Report each funnel stage separately. For example, a model can have high build success but low security quality, or high test pass rate with many static security findings.

## How to report the reduced study

Use language such as:

> We selected 60 tasks using stratified sampling over security weakness families, concurrency/network behavior, general correctness, and maintainability. The same task set was evaluated for every model and language. Automated validation covered parsing, compilation, tests, static analysis, dependency/secret checks, and quality metrics. A stratified human review validated a sample of findings and semantic cases. Results are exploratory for rare CWE categories represented by one task and should not be interpreted as complete coverage of all 82 labels in the source dataset.

Include:

- task-selection table and exact task IDs;
- model/language matrix;
- valid-output, build, test, security, and quality funnel counts;
- manual-review precision/false-positive results;
- CWE/category coverage;
- cost and token totals;
- confidence intervals or bootstrap intervals;
- limitations caused by missing local tests/reference solutions and single-file tasks.

## Final recommendation

Use **20 tasks now** to estimate cost and validate the pipeline. After the pilot, use **60 stratified tasks** for the final reduced benchmark. If the budget permits only 48, use it as an exploratory prototype and make the limitations explicit. Do not use a simple first-60 or random-60 selection, because it will overrepresent common algorithm/CWE labels and underrepresent rare security families.