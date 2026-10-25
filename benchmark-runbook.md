# Updated-Model Benchmark Runbook

## 1. Study objective

The repository is based on the earlier benchmark paper and contains 1,000 task files for each of C, C++, Java, and Python. Each JSON task includes a task number, natural-language prompt, reference solutions, optional hints, test cases, source, and tags. The existing runner supports older provider integrations, but it is not yet a controlled modern benchmark.

The study should answer:

> When current coding models are evaluated on the same tasks, languages, prompts, decoding policy, and validation pipeline, how do they differ in functional correctness, security, software quality, cost, and repairability?

The project contribution should be the **validation and evidence pipeline**, not merely replacing old model names with new ones.

## 2. Security prerequisite

Before any API call:

1. Revoke and rotate every credential previously stored in this workspace. Exposed credentials include the root OpenRouter text file, provider credentials in older Python integrations, and a SonarQube token in the historical properties file.
2. Do not paste keys into source files, notebooks, prompts, logs, Git, or chat.
3. Use environment variables or a local ignored file. The repository now contains `.env.example`; copy it to `.env.local` only if the runner loads it, and never commit `.env.local`.
4. Set a provider spending limit, alert, and hard budget before the pilot.
5. Treat generated programs as untrusted: never execute them on the host with unrestricted filesystem, process, or network access.

Example PowerShell setup for a temporary session:

```powershell
$env:OPENROUTER_API_KEY = "paste-the-rotated-key-locally"
$env:SONAR_TOKEN = "paste-a-new-sonarqube-token-locally"
```

For the first pilot, OpenRouter can simplify access to multiple current model IDs through one OpenAI-compatible endpoint. If direct provider APIs are used instead, use the corresponding environment variables in `.env.example`. Record the exact model ID, provider route, date, and API parameters in the run manifest; do not record the key.

## 3. Freeze the experimental design before spending money

Create a `run-manifest.json` for each run with:

- dataset repository commit or immutable archive hash;
- task IDs and language set;
- model IDs and provider route;
- system and user prompt files;
- temperature, top-p, maximum output tokens, seed if supported, and retry policy;
- tool and compiler versions;
- start/end timestamps and budget;
- output schema version.

Do not change models, prompts, task selection, temperature, or analyzers halfway through a comparison. If a correction is necessary, create a new run ID and explain the change.

## 4. Select the current model matrix

Do not hard-code the phrase “latest model.” Model catalogs change. On the date of data collection:

1. Obtain the provider’s official model list or OpenRouter catalog.
2. Select a small, defensible matrix rather than every available model:
   - one frontier general model;
   - one frontier code-specialized model;
   - one strong open-weight or low-cost model;
   - one budget/fast model;
   - optionally one reasoning-oriented model.
3. Keep model families architecturally diverse. Do not count five aliases of the same backend as five independent models.
4. Confirm that every selected model supports the target context length, code output, rate limits, and structured metadata needed by the runner.
5. Pin exact versioned IDs where available. If a provider exposes only an alias, record the alias and the collection date as a limitation.

The original paper used five model families and four languages. A feasible updated study is four or five exact current model IDs, with Python and JavaScript/TypeScript for the initial engineering slice, followed by the original four-language set if budget and tooling permit. For a strict replication claim, retain all four original languages; for a major-project prototype, report the reduced language scope honestly.

## 5. Pilot design for cost estimation

Use a pilot before the full benchmark:

- 20 tasks total: 5 security-sensitive, 5 correctness-sensitive, 5 data/IO tasks, and 5 general algorithm or utility tasks;
- 5 tasks per language if testing four languages, or balanced across the two initial languages;
- 3 to 5 selected models;
- one generation per task/model/language;
- no automated repair in the first pilot;
- run syntax/build checks and a small validation subset immediately.

The pilot must measure:

- request count and retry count;
- input and output tokens from API usage metadata;
- latency and failure rate;
- generated file size and parse/build success;
- test pass rate;
- security and quality findings;
- actual provider cost where returned, otherwise estimated cost from the published price sheet.

Cost estimate:

```text
pilot_cost = sum over requests of
    (input_tokens / 1,000,000 * input_price)
  + (output_tokens / 1,000,000 * output_price)
  + retry_cost
```

Project the full generation cost by strata, not by a single average:

```text
full_generation_cost = pilot_cost
  * (full_task_count / pilot_task_count)
  * (full_model_count / pilot_model_count)
  * (full_language_count / pilot_language_count)
```

Then add separate budgets for repair attempts, semantic review calls, failed/retried requests, and a 15-25% contingency. Never assume the estimate is exact; report actual usage from the run ledger.

## 6. Recommended run phases

### Phase A: Repository and environment validation

1. Check out or archive the dataset at a fixed commit.
2. Validate every JSON task against `dataset/promptDescriptionSchema.json`.
3. Count tasks by language, tag, source, and security category.
4. Check that reference solutions and test cases are available for the selected tasks.
5. Install pinned Python, compiler, Java, Node.js/TypeScript, SonarQube, and CodeQL versions as appropriate.
6. Run the reference solutions through the test harness. A failing reference test is a dataset or harness issue and must be logged before model evaluation.

### Phase B: Generation pilot

1. Use the same user prompt for every model and task.
2. Keep model instructions language-specific and minimal. Do not give one model extra security hints unless that is a separate experimental condition.
3. Request machine-readable metadata separately from code where possible, or store raw response and extracted code independently.
4. Save request ID, model ID, parameters, timestamps, usage, raw response, extracted source, and error status.
5. Apply rate limits and exponential backoff. Do not silently retry with a different model.
6. Stop automatically when the budget ceiling is reached.

### Phase C: Full generation

Run the frozen task/model/language matrix. Use at least two independent samples per condition if the budget allows; one sample measures one draw, not model reliability. If only one draw is affordable, state that limitation and avoid strong claims about stochastic behavior.

### Phase D: Deterministic validation

For every generated artifact:

1. Extract code and reject malformed or mixed-language output without silently repairing it.
2. Run syntax parsing and compilation.
3. Run the supplied unit tests in an isolated working directory.
4. Run type checking and language linters.
5. Run security analyzers: CodeQL where supported, SonarQube, and a language-specific analyzer such as Bandit for Python or Semgrep rules for supported languages.
6. Run dependency and secret scanning where a project has dependencies/configuration.
7. Record raw findings before deduplication and normalized findings after correlation.

### Phase E: Human validation sample

Automated tools are not ground truth. Sample findings using strata for language, model, severity, analyzer, and CWE/quality category. Have at least two reviewers independently label each sampled finding as true positive, false positive, duplicate, or needs more context. Resolve disagreements with a third review or consensus rule fixed in advance.

### Phase F: Repair experiment

Only start repair after the baseline outputs are frozen. For each selected finding, give the repair model the finding evidence, relevant code, behavior contract, and allowed files. Do not provide the analyzer's hidden answer beyond the normalized evidence being tested.

Apply each patch in a temporary copy and run:

1. parse/build/type gates;
2. original tests plus added regression tests;
3. security and quality scans;
4. targeted negative/security tests;
5. diff and scope checks.

Accept only if behavior is preserved, the target finding is fixed or downgraded with evidence, no unacceptable new finding appears, and the quality/risk score improves. Otherwise record `rejected` with an explicit reason. `needs_human_review` is a valid outcome.

## 7. Security and quality evaluation funnel

Use this funnel in the paper and in the implementation:

```text
Task contract and expected behavior
        |
        v
Generated artifact integrity
  (correct language, complete output, parseable files)
        |
        v
Build and type gate
  (syntax, compilation, imports, type errors)
        |
        v
Functional gate
  (reference tests, edge cases, property/fuzz tests where applicable)
        |
        v
Security gate
  (SAST, taint/data flow, dependency/configuration, secrets, negative tests)
        |
        v
Quality gate
  (lint, complexity, duplication, maintainability, reliability, performance signals)
        |
        v
Independent review sample and finding adjudication
        |
        v
Repair and re-validation, if enabled
        |
        v
Accepted / rejected / needs human review
```

### Security metrics

- CWE/category coverage and counts per 100 artifacts or 1,000 lines;
- true-positive precision and recall on the labeled sample;
- false-positive rate and analyzer disagreement;
- severity-weighted risk score;
- vulnerable artifact rate;
- security regression rate after repair;
- number of new findings introduced by accepted-looking patches.

### Quality metrics

- syntax/build success;
- test pass rate and edge-case pass rate;
- type-error rate;
- maintainability/reliability/code-smell counts;
- cyclomatic or cognitive complexity;
- duplication and dead-code indicators;
- dependency risk and hard-coded secret findings;
- latency/resource behavior when relevant.

Do not combine all findings into one unexplained score. Report raw metrics first, then define any composite score with weights, normalization, and a sensitivity analysis.

## 8. Result tables for the paper

Prepare these tables before running the full experiment:

1. **Model and environment table:** exact model IDs, provider, context, parameters, date, and cost rates.
2. **Dataset table:** task count by language, category, source, and security relevance.
3. **Generation table:** valid output, build success, test pass, mean tokens, latency, and cost.
4. **Security table:** findings by model, language, CWE/category, severity, and normalized rate.
5. **Quality table:** smell, complexity, reliability, duplication, and type-check metrics.
6. **Repair table:** attempted repairs, fixed findings, regressions, accepted patches, rejected patches, and human-review cases.
7. **Ablation table:** raw generation versus guidance-only versus analysis-only versus ungated repair versus complete evidence-gated loop.
8. **Error analysis table:** representative failures, why each validator missed or caught them, and the final disposition.

Use confidence intervals or bootstrap intervals for proportions and report per-language/per-model results, not only averages. Use paired comparisons because every model should receive the same task set. Avoid claiming that a model is “more secure” from a raw scanner count without accounting for output length, analyzer coverage, and false positives.

## 9. How to write the paper contribution

Structure the paper around a measurable system contribution:

1. **Motivation:** current models produce code that can be functional yet insecure or low quality.
2. **Gap:** prior work measures outputs or guides generation, but lacks an independent, cross-language, post-generation acceptance loop.
3. **Research questions:** model/language defect differences; whether evidence-gated repair improves security and quality; cost and regression trade-offs.
4. **Method:** fixed dataset, exact current model IDs, controlled generation, layered validators, human-labeled sample, and frozen repair protocol.
5. **Results:** report raw baseline, pilot-informed full experiment, ablations, confidence intervals, and cost.
6. **Discussion:** identify which defect classes are easy, which remain semantic/system-level, and where analyzers disagree.
7. **Limitations:** model drift, API pricing, dataset age, single-file bias, analyzer blind spots, incomplete tests, and limited human review.
8. **Conclusion:** make only the claim supported by the complete evidence chain.

## 10. Stop/go criteria

Proceed from pilot to full run only if:

- credentials are rotated and absent from tracked files;
- every pilot request has usage and cost data;
- task selection is balanced and reproducible;
- reference tests pass or known fixture defects are documented;
- at least one complete model/language path produces analyzable output;
- the budget projection fits the approved cap;
- generated code execution is isolated;
- the output schema can be reloaded without manual spreadsheet editing.

If a model has high failure or malformed-output rates, keep it as a result rather than silently replacing it. A failed generation is part of the model's benchmark behavior.