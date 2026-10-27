# Paper audit: draft (`Latex_paper.txt`) vs. what was actually done

The draft file holds three LaTeX documents. The third (lines 711–1761) is the latest, and this audit refers to it. The new paper is `paper/main.tex`; the compiled PDF is `paper/build/main.pdf`. `Latex_paper.txt` is unchanged.

## 1. Claims not supported by the experiments (removed or reframed)

| Draft claim | Status | Action |
|---|---|---|
| ContractGuard framework: profiler, obligation compiler, skeleton binder, oracle forge, slot mutation, slot-scoped repair, ledger (abstract, contributions, Sec. III–IV, Fig. 1, Alg. 1, worked example, implementation) | Not implemented and not evaluated | Removed. One clearly labelled future-work paragraph says it was designed but not implemented or evaluated, and that no result concerns it. |
| RQ2–RQ6, baselines B0–B8, ablations, BaxBench/CWEval/custom FastAPI benchmarks, feature sequences, defense survival | Not run | Removed. New RQ1–RQ5 match what was run. |
| "50-task subset" | We used 40 tasks | Corrected (stratified, judgment-based selection; stated as a limitation). |
| "keep the base study's prompt wording and temperature of 0.9" | Our own instruction; temperature 0.2, top-p 0.95, max 4096 tokens, ≤3 attempts | Corrected; the threats section says the prompt is not the base study's. |
| "SonarQube and CodeQL findings" | Not run | Replaced by what was run: Semgrep (pinned packs), Bandit, Flawfinder, Cppcheck, Lizard. Threats section says CodeQL and SonarQube were not run. |
| "unit-test pass rate where tests exist" | Partly. The recovered tests needed an audit and adapters; 130 combinations were executed | Reported with the full coverage funnel and the saturation caveat. |
| Flagship-tier comparison | Not run | Removed; stated as an external-validity limit. |
| "every configuration runs with at least three seeds; bootstrap CIs; Holm correction; 10% manual audit of secure verdicts" | Not done | Replaced by what was done: one sample per combination, Wilson CIs, exact McNemar (exploratory, unadjusted), AI-assisted triage of 65 findings. |
| Positioning table claiming ContractGuard has all properties | Unsupported | Removed. |
| Contribution "replication of the base study" with comparable numbers | Prompt and tool differences prevent a numeric comparison | Only a qualitative comparison (language direction) is made, citing the base study's abstract. |

## 2. Related-work numbers checked against the abstracts (arXiv API, `paper/refcheck/`)

| Reference | Draft | Verified (abstract) | New paper |
|---|---|---|---|
| SusVibes | 61.0% functional / 10.5% secure | 57% / 11.8% (one agent configuration) | Uses 57% / 11.8% |
| Tessa et al. | 7.4–21.6× overstatement; 3.0–13.3% | 7 to 21×; 3 to 17% | Uses 7–21× |
| BaxBench | "best correct-and-secure ≈35–37%" | Not in abstract: o1 62% correctness, exploits on ~half of correct programs | Uses the abstract figures |
| Dai et al. | "10–30% of solved tasks broke" | "many techniques degrade the base LLM by more than 50%" | Qualitative statement only |
| CSDD | "violations fell from 11 to 3" | "reduce security defects by 73%" | Uses 73% |
| "sastlabels" (placeholder title) | 65% / 61% agreement | Firouzi & Ghafari, "Persistent Human Feedback, LLMs, and Static Analyzers…"; 65% / 61% confirmed | Title and authors fixed |
| MA-CoT (no authors) | — | Kharma, Alkhanafseh, Sabbah, Mohaisen (the base study's authors) | Authors added |
| METR | "19% slower, believed 20% faster" | Confirmed: completion time +19%; developers estimated afterwards a 20% reduction | Uses these figures |
| SoK (Patir et al.) | 93.7–96.5%, 47.9 and 72.9 points | Numbers not in the abstract | Numbers not used |
| MT-Sec | 20–27% | Confirmed (arXiv version) | Used, citing arXiv |

The base study (Kharma et al., IEEE TDSC 23(3):7300–7314, 2026, doi 10.1109/TDSC.2026.3672745) and SecureCoder (Zhao et al., IEEE TSE 51(12):3507–3523, 2025) were checked against the PDFs in the repository.

References dropped because the new paper no longer cites them (they supported the ContractGuard design): endor2026, brace2026, scgagent2025, testsasspec2026, blyth2025, pocgym2026, autobax2025, threatmodel2026, autocedar2026, authprobe2026, evocode2026, srcode2026, secvec2026, codeguardplus2024, meyer1992, jia2011. Most of them carried "TODO: verify" notes in the draft.

## 3. How consistency is enforced

- Every result number in `main.tex` is a macro from `generated/numbers.tex`. Every table is `generated/tab_*.tex` and every figure is `figures/*.pdf`. All three are produced by `scripts/make_assets.py`, which reads only the evaluation outputs and `paper/data/triage_ai_proposed.csv`.
- `scripts/check_paper.py` lists every literal number left in the text, plus cite/bib and ref/label mismatches. At the last build: no mismatches, no undefined references, no overfull boxes, 7 pages.

## 4. Decisions needed from the authors

1. **Title.** The registered project title describes the unimplemented framework. `main.tex` uses an empirical title and keeps the old one in a comment.
2. **Human confirmation of AI-assisted decisions:**
   - 76 oracle decisions (`evaluation/functional/oracle_review.json`);
   - 174 adapter decisions (`adapter_review.json`);
   - 65 triage labels (`paper/data/triage_ai_proposed.csv`, `human_label` column).
   The paper states that these are unconfirmed. Once confirmed, rerun `make_assets.py` and `build.sh`.
3. **Author block, affiliation, e-mails, acknowledgment, repository URL** (TODO comments in `main.tex`).
4. **Target venue.** The paper uses IEEE conference format, currently 7 pages.
5. **Whether to keep the ContractGuard future-work paragraph.**
