# Comparing with the base paper's outputs on the same 40 tasks: assessment and proposed protocol

Status: **approved 2026-10-27 (Java excluded from the primary analysis) and run.** Results: `evaluation/results/base-40-comparison-v1/RESULTS.md`; pipeline outputs: `evaluation/results/base-40-published-v1/`. One post-hoc analysis (S4, symmetric integrity) was added and is labelled as not pre-declared. Assessment evidence comes from read-only scripts in this folder: `classify_base_versions.py` → `base_version_classes.csv`, and `count_escape_artifacts.py`.

## 1. Which base-paper outputs exist for the 40 tasks

| Source | Languages | Files (40 tasks × 5 models) | What it is |
|---|---|---|---|
| `Repo/…/dataset/output/<MODEL>/` ("published") | C, C++, Python | 200 each | Outputs released with the dataset. **No Java.** |
| Base test repositories, pinned commits (`evaluation/.upstream/…`) ("test copies") | C, C++, Python, Java | 200 each | Copies used by the base paper's unit tests |

## 2. Which version the base paper analysed

The base paper's own CodeQL and SonarCloud reports (`Repo/…/python/analysis/**.xlsx`) reference **test-repository paths**: `source/output/<model>/c|cpp/`, `output/<model>/` (Python), and `src/main/java` (plus some `src/test/java`) for Java. The CodeQL C++ report even includes a vendored OpenSSL tree under `tests/claude/Task102/`. Its static analysis therefore ran on the test copies, not on the published outputs. (`sonar-project.properties` points at `./dataset/output`, but the SonarCloud reports in the repository use the test-repository paths.)

## 3. Can the 73 manually fixed files be identified?

**No, not individually.** The repository contains no fix log or list: no file name, script or log mentions the manual fixes, and `python/logs/` only holds Task 185. What can be measured is how the published outputs differ from the test copies (`base_version_classes.csv`, 600 files):

| Class | C | C++ | Python | Meaning |
|---|---|---|---|---|
| identical | 139 | 110 | 190 | No post-processing |
| whitespace only | 1 | 1 | 0 | |
| harness: `main` removed | 0 | 44 | 1 | Test adaptation |
| harness: lines commented out | 27 | 21 | 3 | Test adaptation (usually `main`/example code) |
| escape artifact fixed | 4 | 0 | 0 | Compile-enabling fix (e.g. `'\\0'` → `'\0'`) |
| other edit | 29 | 24 | 6 | Mixed; see below |

By model, non-identical files: OpenAI 47, Claude 36, Perplexity 34, Mistral 22, Gemini 20.

A sample of "other edit" shows three kinds of change:
- compile fixes (added `#include`s);
- API corrections beyond syntax, e.g. Mistral T130 C: `O_THRPOOL` → `O_POOL` and changed `onion_listen` arguments;
- logic changes, e.g. OpenAI T68 C++: an added argument check and a rewritten LDAP query construction.

These exceed the paper's description of strictly syntactic fixes. **Conclusion:** the test copies, which the base paper analysed, are not model output. The published outputs are the closest available proxy for raw model output, but:
- they may themselves include some of the 73 fixes;
- 37 published C/C++ files contain doubled-backslash escapes (`count_escape_artifacts.py`), a likely extraction artifact of the base pipeline. 23 of them are identical in the test copies, so the pattern cannot separate artifacts from intended code;
- Java has no published version at all.

## 4. Proposed fair comparison protocol (for approval)

1. **Objects.** Base: the **published** outputs for C, C++ and Python (600 files). Ours: the frozen snapshot `30d64f1cb11b…`, same 40 tasks and languages. Java is excluded from the primary analysis because only post-processed copies exist; optionally it is reported separately and flagged.
2. **Pipeline.** Identical for both cohorts:
   - `evaluate.py` gate 1 (same toolchain, flags and timeouts);
   - `scan.py` (same tool versions and the same pinned rule hashes);
   - consolidation v2.
   Base files are hashed into their own snapshot manifest and copied read-only. **No edits, no fixes, no harness changes.** Results go to new folders (`results/base-40-published-*`).
3. **Primary measures.** Gate 1 pass rate, share of compilable files with ≥1 medium/high potential finding, NLOC, CCN and CCD, and Cppcheck/KLOC (C/C++). Each per cohort × language with Wilson CIs.
4. **Comparison unit.** Cohort-level, per language. Each (task, language) cell can additionally be paired across cohorts by the number of the 5 models passing (Wilcoxon signed-rank as a sensitivity check). Model-to-model pairing across cohorts is not meaningful.
5. **Sensitivity analyses** (all pre-declared):
   - (a) only files classed **identical** (439 files, untouched by any post-processing);
   - (b) excluding the 37 C/C++ files with doubled-backslash escapes;
   - (c) gate 1 with and without counting "blocked" (missing third-party headers) as failures.
6. **Not compared:**
   - functional tests (the base tests are per-model and our functional set is different);
   - the base paper's published CodeQL/SonarQube numbers (different tools, post-processed inputs);
   - Java in the primary analysis.
7. **Reporting.** Differences are attributed to **cohorts** (models plus generation protocol), never to models alone. The confounders are:
   - prompt wording;
   - temperature 0.9 vs 0.2;
   - output-token limits;
   - retry policy;
   - each pipeline's code extraction (theirs shows escape artifacts; ours validates raw source);
   - generation dates about 1.5 years apart.

## 5. Decisions needed before running

- Approve or modify the protocol (especially items 1 and 5).
- Whether to include Java as a flagged secondary analysis using the test copies.
- Whether the comparison goes into the paper as a descriptive cohort comparison, or into supplementary material only.
