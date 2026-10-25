# Research Synthesis and Proposed Solution

## Source status

Two supplied papers were readable and reviewed. `Major Project.pdf` is a zero-byte file, so it contains no abstract or project-specific requirements to analyze. The project scope below is therefore derived from the two papers and the user's stated requirement for an actual coding-quality solution.

## Paper comparison

| Source | What it contributes | Evidence and scope | Important limitation |
|---|---|---|---|
| *Security and Quality in LLM-Generated Code: A Multi-Language, Multi-Model Analysis* | A broad measurement baseline for security and quality differences between languages and models. | 200 manually curated tasks, 4 languages (Python, Java, C++, C), 5 LLM families, 4,000 generated files; compilation/syntax checks, 4,000 unit-test files, SonarQube, CodeQL, and manual review. | Primarily single-turn, single-file generation. It measures defects but does not build an automated repair and acceptance loop. Static tools can miss semantic and system-level defects; tests can miss untested behavior. |
| *Towards Secure Code Generation With LLMs: A Study on Common Weakness Enumeration* | SECURECODER: CWE-vector retrieval, requirement summarization, security guidance, and guided regeneration. | Evaluated 12 LLM configurations and 18 CWE categories using LLMSecEval, Big-Vul, and a vulnerable-code dataset; reports improved secure-generation rates and functional consistency using CodeBLEU and HumanEval pass@1. | It is still largely guidance- and model-mediated. Evaluation is restricted mainly to C and Python and 18 CWE categories. It does not establish deterministic post-generation evidence, sandboxed execution, regression gates, or general code-quality verification. |

## What the papers establish

1. Functional correctness is not a security guarantee. Code can compile and pass available tests while retaining security hotspots.
2. Language choice materially changes the defect distribution. C and C++ expose more memory-safety and low-level misuse risks, while Python and Java generally show stronger semantic success in the baseline study.
3. Models differ in both correctness and security posture. A single model ranking is not a sufficient universal answer.
4. CWE knowledge and structured guidance can reduce some weaknesses before or during regeneration.
5. Semantic/business-logic vulnerabilities remain difficult because they may not be visible to a traditional scanner and may occur in incomplete snippets.
6. Model reasoning and instruction-following failures can cause a security-guided regeneration step to change the original behavior.
7. Static analyzers are valuable but incomplete. The baseline paper explicitly identifies false positives and limited semantic reasoning as validity concerns.
8. Reproducible evaluation requires fixed model versions, controlled prompts, language-specific build environments, tests, and recorded configurations.

## Research gap for this project

The useful gap is not “ask an LLM to write safer code.” It is a **verified engineering system** that combines prevention, detection, repair, and acceptance:

- Existing measurement work tells us where failures occur, but not how to automatically prevent and safely repair them.
- CWE-guided generation improves the input to a model, but a model-generated explanation or repair is not independent evidence.
- Existing static analysis is language/tool specific and has incomplete coverage; one scanner cannot be treated as ground truth.
- Existing tests provide functional evidence only for covered behavior and are insufficient as the sole security gate.
- Code similarity metrics such as CodeBLEU do not prove behavioral equivalence or absence of new vulnerabilities.
- Many evaluations focus on snippets or single files, while real risk crosses dependencies, configuration, trust boundaries, and modules.
- A practical system needs explicit failure states: “not analyzable,” “needs human review,” and “repair rejected,” rather than silently claiming success.

## Proposed research contribution

Build a **Verified Code Quality and Security Loop (VCQSL)** with five engineering properties:

1. **Evidence-first analysis:** Every finding must point to source evidence, analyzer/rule, confidence, severity, and affected behavior or data flow.
2. **Defense in depth:** Combine parser/type/build checks, deterministic lint and SAST rules, dependency/configuration checks, tests, targeted fuzz/property checks, and optional model-assisted semantic review.
3. **Constrained repair:** Repairs are generated for a specific finding and must preserve an explicit behavioral contract. They cannot be accepted merely because the model says they are fixed.
4. **Independent acceptance:** A patch is accepted only when required tests pass, the original finding is resolved or downgraded with evidence, no unacceptable new finding appears, and the risk/quality score improves.
5. **Cross-language adapters:** The core finding, evidence, patch, and verification contracts are language-neutral; language adapters own parsing, build, testing, and analyzer details.

## Reference architecture

```text
Task and behavior contract
        |
        v
Model adapter(s) -> candidate project
        |
        v
Normalizer: parse, format, dependency lock, build metadata
        |
        +--> Static layer: AST, types, taint/data flow, SAST, lint, dependencies
        +--> Dynamic layer: unit/integration tests, property tests, fuzz probes
        +--> Runtime layer: isolated sandbox, time/memory/network policy
        |
        v
Finding correlator and risk scorer
        |
        +--> Report with evidence and recommended repair plan
        +--> Repair adapter proposes minimal patch
                         |
                         v
                 Temporary patch workspace
                         |
                         v
              Re-run all applicable gates and compare
                         |
             accept / reject / human review required
```

## Core data contracts

### Finding record

`finding_id`, `language`, `file`, `location`, `rule_id`, `CWE` or quality category, `severity`, `confidence`, `evidence`, `data_flow`, `exploitability`, `repairability`, `analyzer`, `status`, and `verification_results`.

### Patch record

`patch_id`, `parent_finding_ids`, `diff`, `changed_files`, `behavior_contract`, `pre_metrics`, `post_metrics`, `test_results`, `security_results`, `quality_results`, `rejection_reasons`, and `acceptance_decision`.

### Acceptance policy

Reject a patch when it fails to build, violates tests or the behavior contract, leaves a high-confidence critical finding unresolved, introduces a higher-risk finding, weakens validation to silence a rule, or cannot be reproduced. Send it to human review when analyzers disagree or evidence is incomplete.

## Recommended implementation scope

### Minimum viable research prototype

- Languages: Python and JavaScript/TypeScript, or Python and Java if the project environment already supports Java tooling reliably.
- Interface: CLI plus JSON/HTML reports.
- Detection: syntax/type checks, one SAST engine per language, dependency scanning, complexity/quality rules, and test execution.
- Repair: one model adapter that receives only a normalized finding and relevant code context.
- Verification: pre/post scans, tests, diff review, and a risk-based acceptance gate.
- Benchmark: 30 to 60 tasks, balanced across clean code, seeded defects, and generated code; start with 8 to 12 security categories plus non-security quality cases.

### Extensions after the vertical slice

- C/C++ memory-safety analysis and compiler sanitizers.
- Multi-file dependency and configuration analysis.
- Property-based testing and fuzzing.
- Business-logic contracts and authorization-flow checks.
- Multiple model providers and cost/latency analysis.
- IDE integration and human reviewer feedback.

## Evaluation design

Compare these conditions on the same tasks and model seeds where possible:

1. Raw model generation.
2. Prompt-only self-review or secure-generation guidance.
3. Deterministic analysis without repair.
4. Analysis plus model-assisted repair without acceptance gates.
5. Complete VCQSL loop.

Primary metrics:

- Syntax/build success rate.
- Functional pass rate and regression rate.
- Security precision, recall, false-positive rate, and CWE coverage.
- Quality score change, complexity change, and maintainability indicators.
- Repair success rate and patch acceptance rate.
- New-defect introduction rate.
- Time, token/API cost, and number of verification iterations.
- Human-review agreement on a stratified sample.

The central claim should be modest and testable: **the complete evidence-gated loop produces a higher proportion of functionally valid, lower-risk code than raw generation and guidance-only baselines under the same evaluation conditions.**

## Threats to validity and controls

- **Tool blind spots:** use multiple analysis layers and manually validate a stratified sample.
- **Incomplete tests:** add mutation testing, property checks, and explicit behavior contracts where feasible.
- **Model/version drift:** pin versions, prompts, parameters, timestamps, and API metadata.
- **Benchmark leakage:** keep an unseen holdout set and report seeded versus naturally occurring defects separately.
- **Language imbalance:** report per-language results instead of only aggregate scores.
- **False repair confidence:** require independent re-analysis and make rejection a first-class outcome.
- **Single-file bias:** begin with a controlled slice but include a multi-file milestone before claiming system-level validity.
- **Unsafe execution:** run generated code with resource, filesystem, process, and network restrictions.

## Final project position

The strongest major-project framing is not a new LLM and not a prompt template. It is a **model-agnostic verification and repair pipeline** that makes generated code accountable to independent evidence. The novelty can be demonstrated through the normalized cross-language evidence model, repair acceptance policy, and controlled comparison against raw generation and prompt-only baselines.