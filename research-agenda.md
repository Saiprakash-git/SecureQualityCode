# Major Project Research Agenda

## Working title

**A Multi-Language, Multi-Model System for Detecting, Explaining, Repairing, and Validating Security and Quality Defects in LLM-Generated Code**

## Problem definition

Large language models can produce code that compiles and appears plausible while containing security weaknesses, reliability defects, poor maintainability, and language-specific mistakes. Prompting alone is not a sufficient solution because it does not provide deterministic detection, evidence, regression protection, or a measurable guarantee that a generated repair is better than the original.

## Central research question

Can a tool-assisted closed-loop coding system reduce security and quality defects in LLM-generated programs across languages and models while preserving functional behavior, and can its improvements be demonstrated with reproducible benchmarks?

## Aims and goals

1. Read and critically compare the base paper, the related CWE-focused study, and the project abstract.
2. Build a taxonomy connecting security weaknesses, general code-quality defects, language features, model behavior, and repairability.
3. Identify gaps in existing LLM code-generation, security-scanning, repair, and evaluation approaches.
4. Define a system architecture that is more than a prompt wrapper: generation, normalization, static analysis, semantic checks, tests, sandbox execution, repair planning, patching, and regression validation.
5. Support at least two programming languages initially, with an extensible analyzer interface for additional languages.
6. Compare multiple code-generation models under controlled prompts and identical evaluation conditions.
7. Detect both security issues and quality issues, including correctness, reliability, maintainability, and dependency/configuration risks.
8. Produce actionable explanations with file/line evidence, weakness category, severity, confidence, and suggested remediation.
9. Generate constrained repairs and reject patches that introduce new findings, break tests, or change required behavior.
10. Measure precision, recall, false positives, functional pass rate, security improvement, quality improvement, patch acceptance, and repair cost.
11. Create a reproducible dataset and evaluation harness with clean, vulnerable, repaired, and adversarial cases.
12. Document limitations, threats to validity, ethical boundaries, and a practical roadmap for a major-project implementation.

## Proposed system direction

The candidate solution is a **Verified Code Quality and Security Loop**:

```text
Task specification
      -> candidate generation by one or more models
      -> parsing, formatting, dependency and build normalization
      -> language-aware static analysis and security rules
      -> type checking, tests, fuzz/property checks where applicable
      -> isolated sandbox execution and resource limits
      -> finding correlation, ranking, and explanation
      -> constrained repair proposal
      -> patch application in a temporary branch
      -> repeat analysis and tests
      -> accept only if behavior is preserved and the risk/quality score improves
```

The research contribution should be the orchestration, evidence model, cross-language abstraction, repair acceptance policy, and evaluation methodology. LLMs may propose code or patches, but they should not be the sole authority for deciding whether code is safe.

## Research workstreams

### 1. Literature and source-paper review

- Extract each document's claims, methods, datasets, metrics, findings, and limitations.
- Separate evidence from proposed future work.
- Build a comparison table between the base paper, related study, and this project.

### 2. Defect taxonomy

- Map security findings to CWE categories and language-specific variants.
- Add non-security quality dimensions: correctness, robustness, readability, complexity, testability, dependency hygiene, and performance risks.
- Define severity, confidence, exploitability, and repairability fields.

### 3. System design

- Define adapters for generation models, parsers, linters, SAST tools, test runners, and sandboxes.
- Define a normalized finding schema and a patch/evidence record.
- Design model-independent acceptance gates.

### 4. Detection and analysis

- Combine deterministic rules, AST/data-flow analysis, type checking, dependency scanning, and tests.
- Add semantic or model-assisted analysis only where deterministic methods are insufficient.
- Correlate duplicate findings and rank actionable risks.

### 5. Repair and verification

- Generate minimal patches tied to specific findings.
- Run pre/post scans, tests, type checks, and behavior comparisons.
- Reject regressions, incomplete repairs, and patches that merely suppress warnings.

### 6. Evaluation

- Establish baselines: raw model output, prompt-only self-review, static-analysis-only, and the complete loop.
- Evaluate across languages, models, task types, and defect categories.
- Include unseen tasks and seeded/adversarial defects to test generalization.

### 7. Prototype and usability

- Provide CLI and report output first; add an IDE or web interface only after the engine is reliable.
- Show evidence, severity, explanation, patch diff, verification results, and rejection reasons.
- Record reproducibility metadata without exposing secrets or unsafe execution.

## Progress board

- [x] Workspace and source-document inventory
- [x] Extract and read the two non-empty research PDFs
- [x] Record that `Major Project.pdf` is empty and needs to be re-supplied
- [x] Write source-paper comparison and evidence table
- [x] Complete initial problem taxonomy and research gap analysis
- [x] Define initial pilot/full-run benchmark scope and validation funnel
- [ ] Select exact current model IDs after provider catalog freeze
- [ ] Specify normalized finding and verification schemas
- [ ] Design prototype architecture and acceptance gates
- [ ] Implement a minimal end-to-end vertical slice
- [ ] Add repair loop and regression rejection
- [ ] Build benchmark runner and baseline experiments
- [ ] Run repeated validation and analyze failure cases
- [ ] Document results, limitations, and future extensions
- [ ] Produce final project proposal/report structure

## Initial success criteria

- The system can analyze generated code in at least two languages.
- It reports findings with evidence rather than unsupported model assertions.
- It detects a meaningful set of seeded security and quality defects.
- It can propose repairs and automatically reject unsafe or behavior-breaking patches.
- The complete loop outperforms raw generation and prompt-only review on predefined metrics.
- Experiments are repeatable from a documented configuration and dataset.

## Validation loop

For every implementation increment:

1. Run unit tests for the changed analyzer or orchestration component.
2. Run the smallest representative vulnerable/clean fixture set.
3. Run type, syntax, build, and security checks where applicable.
4. Compare pre/post findings and test outcomes.
5. Add a regression fixture for every confirmed failure.
6. Re-run the complete benchmark at milestone points and record results.

## Immediate next actions

1. Rotate all credentials previously exposed in the workspace and remove the root key text file.
2. Freeze the exact current model IDs, prices, parameters, and task strata in a run manifest.
3. Run the 20-task pilot and record usage, cost, failure rate, and validation results.
4. Choose the first vertical slice, likely Python plus JavaScript/TypeScript or Java, based on available analyzers.
5. Implement detection, evidence reporting, and validation before adding automated repair.