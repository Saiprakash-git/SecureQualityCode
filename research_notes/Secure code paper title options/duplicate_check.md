# Title Duplicate / Collision Check for Candidate Paper Titles

Scope: existing papers (2022–2026) whose titles overlap with our four candidate titles. Search done 2026-10-27 using web search (about 15 queries) across arXiv, IEEE/CSDL, ResearchGate, ACL Anthology and aggregators. I did not query dblp, Google Scholar, ACM DL or IEEE Xplore directly. Many hits are known only from search snippets or abstract pages. Author lists are given only where a source showed them.

## Q1: Which existing titles overlap with our key phrases?

### Takeaway
No existing title found combines "CWE" with "business rules/business logic" for secure code generation. Our closest collisions are (a) the Kharma et al. base paper itself, which candidate 2 echoes closely; (b) "Secure Code Generation" titles, a very crowded group; and (c) SecureCoder (IEEE TSE 2025), which uses CWE plus a "business logic" description step in its method, though not in its title.

### Cited Findings

**Very close / direct echoes**
- "Security and Quality in LLM-Generated Code: A Multi-Language, Multi-Model Analysis" — Kharma, Choi, AlKhanafseh, Mohaisen (UCF / Birzeit), arXiv 2502.01853 (2025), IEEE TDSC version per our context. This is the paper we replicate. Candidate 2 ("Revisiting the Security and Quality of LLM-Generated Code") reuses its core phrase. That is fine for a replication, but readers will see it as a direct follow-up. — [arXiv](https://arxiv.org/abs/2502.01853)
- "Assessing the Quality and Security of AI-Generated Code: A Quantitative Analysis" — arXiv 2508.14727 (2025). Java, SonarQube, 4,442 assignments. It uses the same "quality and security" pair. — [arXiv](https://arxiv.org/abs/2508.14727)
- "WildCode Revisited: A Comprehensive Empirical Study on the Security of LLM-Generated Code" — arXiv 2512.04259. This is the only "Revisited + security of LLM-generated code" title found. It is close to candidate 2's opening word and theme. — [arXiv](https://arxiv.org/pdf/2512.04259)
- "Enhancing Reliability in LLM-Based Secure Code Generation" (MA-CoT) — Kharma, Alkhanafseh, Sabbah, Mohaisen, arXiv 2605.24300 (May 2026). Same group as the base paper. It embeds CWE mitigation guidance into prompting. It is very close in method to the CWE half of our framework. — [arXiv](https://arxiv.org/html/2605.24300); [chatpaper](https://chatpaper.com/paper/285683)
- "An Empirical Evaluation of LLM-Generated Code Security Across Prompting Methods" — arXiv 2605.24298 (2026). Same author group (adds Hammoudeh per snippet). Five LLMs × Java/C++/C/Python, plus WA-0CoT CWE-aware prompting. Its model × language setup nearly matches our study (5 models, 4 languages). — [arXiv](https://arxiv.org/abs/2605.24298)

**CWE-knowledge titles**
- "Towards Secure Code Generation With LLMs: A Study on Common Weakness Enumeration" (SecureCoder) — Jianguo Zhao et al., IEEE TSE vol. 51 no. 12, 2025, DOI 10.1109/TSE.2025.3619281. RAG + CWE. In its method, the LLM first describes the code's "business logic", then maps it to CWE guidance. This is the closest existing work to "CWE + business logic", but its business logic means functional intent, not authorization or workflow rules. — [IEEE Xplore](https://ieeexplore.ieee.org/document/11197590/); [CSDL](https://www.computer.org/csdl/journal/ts/2025/12/11197590/2aG1tvzTeec)
- "Activating Latent Security Knowledge through LLM-Guided Risk Analysis for Secure Code Generation" (BRACE) — arXiv 2606.16244 (2026). LLM-identified CWE risks validated against the CWE catalog. — [arXiv](https://arxiv.org/abs/2606.16244)
- "SPARK: Security Knowledge Priming and Representation-Guided Knowledge Activation for LLM-based Secure Code Generation" — seen only as a citation in the BRACE paper (author incl. Xiaoyun Xu). Not verified. — [via BRACE](https://arxiv.org/pdf/2606.16244)
- "CWEval: Outcome-driven Evaluation on Functionality and Security of LLM Code Generation" — arXiv 2501.08200 (2025). — [arXiv](https://arxiv.org/abs/2501.08200)
- "Secure and Functional Code Generation with Self-Evaluation" (SecGent) — arXiv 2610.33450 (2026). Pulls CWE context, CWE-aware security tests. — [arXiv](https://arxiv.org/html/2610.33450v1)
- "Guiding AI to Fix Its Own Flaws: An Empirical Study on LLM-Driven Secure Code Generation" — arXiv 2506.23034 (2025). Vulnerability hints plus repair. — [arXiv](https://arxiv.org/html/2506.23034v1)
- "Improving LLM-Assisted Secure Code Generation through Retrieval-Augmented-Generation and Multi-Tool Feedback" — arXiv 2601.00509 (2026). — [arXiv](https://arxiv.org/pdf/2601.00509)
- "SecV: LLM-based Secure Verilog Generation with Clue-Guided Exploration on [hardware CWE knowledge graph]" — IJCAI 2025 (hardware). — [IJCAI](https://www.ijcai.org/proceedings/2025/0895.pdf)

**Specification / test-driven secure generation (overlaps the framework half)**
- "Constitutional Spec-Driven Development: Enforcing Security by Construction in AI-Assisted Code Generation" — arXiv 2602.02584 (2026). A machine-readable "constitution" drawn from CWE/MITRE Top 25 plus regulations. — [emergentmind](https://www.emergentmind.com/papers/2602.02584); [ResearchGate](https://www.researchgate.net/publication/400414467_Constitutional_Spec-Driven_Development_Enforcing_Security_by_Construction_in_AI-Assisted_Code_Generation)
- "Specification-Driven Development Benchmark: Security Knowledge Transition" — arXiv 2606.00167 (29 May 2026, cs.SE/cs.LO). Authors not retrieved. — [arXiv](https://arxiv.org/pdf/2606.00167); [emergentmind](https://www.emergentmind.com/papers/2606.00167)
- "Security Tests as Executable Specifications for LLM Code Generation: Benefits, Trade-offs, and Coverage Limits" — Yunhao Liang, Chengguang Gan, Ruixuan Ying, arXiv 2608.09740 (2026). SecTDD, with repair/regression transitions. Close to our "checked security tests + targeted repair". — [arXiv](https://arxiv.org/abs/2608.09740)
- "Secure Code Generation at Scale with Reflexion" — Arup Datta et al., arXiv 2511.03898. Tracks regressions across revisions. — [arXiv](https://arxiv.org/pdf/2511.03898)

**"Secure Code Generation" title group (crowded)**
- "SCGAgent: Recreating the Benefits of Reasoning Models for Secure Code Generation with Agentic Workflows" — Saul, Wang, Sen, Wagner, arXiv 2506.07313 (2025). — [arXiv](https://arxiv.org/abs/2506.07313)
- "Rethinking the Evaluation of Secure Code Generation" — arXiv 2503.15554. An alternate version is titled "A Comprehensive Study of LLM Secure Code Generation". — [arXiv](https://arxiv.org/abs/2503.15554)
- "SoK: AI Secure Code Generation: Progress, Pitfalls, and Paths Forward" — arXiv 2606.25195 (2026). — [arXiv](https://arxiv.org/html/2606.25195v1)
- "Toward Secure Code Generation: Bridging Correctness and Security via Task-Adaptive Vulnerability Modeling and Execution-Based Benchmarking" — arXiv 2407.02395. — [arXiv](https://arxiv.org/pdf/2407.02395)
- "Persistent Human Feedback, LLMs, and Static Analyzers for Secure Code Generation and Vulnerability Detection" — arXiv 2602.05868. — [arXiv](https://arxiv.org/html/2602.05868v1)
- "RealSec-bench: A Benchmark for Evaluating Secure Code Generation in Real-World Repositories" — arXiv 2601.22706. — [arXiv](https://arxiv.org/abs/2601.22706)
- "SecRepoBench: Benchmarking LLMs for Secure Code Generation in Real-World Repositories" — arXiv 2504.21205. — [arXiv](https://arxiv.org/html/2504.21205v1)
- "SecureAgentBench: Benchmarking Secure Code Generation under Realistic Vulnerability Scenarios" — arXiv 2509.22097. — [arXiv](https://arxiv.org/pdf/2509.22097)
- "Security-by-Design for LLM-Based Code Generation: Leveraging Internal Representations for Concept-Driven Steering Mechanisms" — arXiv 2603.11212. — [arXiv](https://arxiv.org/html/2603.11212v1)
- "TypePilot: Leveraging the Scala type system for secure LLM-generated code" — arXiv 2510.11151. — [arXiv](https://arxiv.org/html/2510.11151v1)
- "Understanding and Improving Model Editing for Secure Code Generation" — 2608.06848 (seen only via a review site). — [pith.science](https://pith.science/paper/2608.06848)
- "SecCoder: Towards Generalizable and Robust Secure Code Generation" — EMNLP 2024. — [ACL Anthology](https://preview.aclanthology.org/setup/2024.emnlp-main.806)
- "Enhancing Large Language Models for Secure Code Generation: A Dataset-driven Study on Vulnerability Mitigation" — arXiv 2310.16263. — [ar5iv](https://ar5iv.labs.arxiv.org/html/2310.16263)

**Generic "security of LLM-generated code" studies**
- "Security of LLM-generated Code: A Comparative Analysis" — arXiv 2605.23091. — [arXiv](https://arxiv.org/abs/2605.23091)
- "Security Vulnerability Patterns in AI-Generated Code: A Cross-Model Comparative Study" — arXiv 2607.20713. — [arXiv](https://arxiv.org/html/2607.20713)
- "Poster: Rethinking Security in LLM Code Generation through Real-World Risk Scenarios" — arXiv 2607.23088. — [arXiv](https://arxiv.org/abs/2607.23088)
- "A.S.E: A Repository-Level Benchmark for Evaluating Security in AI-Generated Code" — arXiv 2508.18106. Its abstract says the target code is tied to project business logic. — [arXiv](https://arxiv.org/html/2508.18106v2)

**Benchmarks named in the brief**
- BaxBench: "BaxBench: Can LLMs Generate Correct and Secure Backends?" — Vero et al., ICLR 2025 (title per search summary). — [search result context](https://arxiv.org/pdf/2509.22097)
- SusVibes: "Is Vibe Coding Safe? Benchmarking Vulnerability of Agent-Generated Code in Real-World Tasks" — Zhao et al., 2025 (from ResearchGate; arXiv 2512.03262 per emergentmind). — [ResearchGate](https://www.researchgate.net/publication/398313281_Is_Vibe_Coding_Safe_Benchmarking_Vulnerability_of_Agent-Generated_Code_in_Real-World_Tasks); [emergentmind](https://www.emergentmind.com/papers/2512.03262)
- Other vibe-coding titles: "SecureVibeBench" (ACL 2026), "SecureVibe: Making Vibe Coding More Secure" (arXiv 2609.38606), "Last Secure Code Benchmark: Is the Code You Vibe Secure?" (arXiv 2610.14766). — [ACL](https://aclanthology.org/2026.acl-long.1107.pdf); [arXiv](https://arxiv.org/html/2609.38606v1); [arXiv](https://arxiv.org/html/2610.14766v1)

**Access control / authorization / business logic titles**
- "Repairing Broken Access Control in Web Applications with LLMs: An Empirical Benchmark Study" (BAC-Bench) — arXiv 2610.34290 (2026). Repair, not generation. — [arXiv](https://arxiv.org/html/2610.34290)
- "AuthGateBench: Benchmarking LLMs as Authorization Gate" — arXiv 2610.16426 (Oct 2026). Agent action gating, not code generation. — [arXiv](https://arxiv.org/html/2610.16426v1)
- "Anota: Identifying Business Logic Vulnerabilities via Annotation-Based Sanitization" — arXiv 2512.20705. Detection, not LLM code generation. — [arXiv](https://arxiv.org/pdf/2512.20705)
- "Compiled AI: Deterministic Code Generation for LLM-Based Workflow Automation" — arXiv 2604.05150. Uses "business logic" in the sense of workflow functions. — [arXiv](https://arxiv.org/abs/2604.05150)
- "On LLM-Assisted Generation of Smart Contracts from Business Processes" — arXiv 2507.23087. — [arXiv](https://arxiv.org/pdf/2507.23087)
- "Improving LLM Code Generation via Requirement-Aware [Curriculum RL]" (RECRL) — arXiv 2605.00433. Not security. Title truncated in the snippet. — [arXiv](https://arxiv.org/pdf/2605.00433)

### Inferences
- Candidate 2 is the most collision-prone. "Security and Quality of LLM-Generated Code" is nearly verbatim the base paper's title, and "Revisiting" plus security of LLM-generated code already appears in "WildCode Revisited". It is acceptable as an explicit replication signal, but it is not distinctive.
- Candidates 1, 3 and 4 have no near-duplicate title. Their overlap is limited to the generic stem "Secure (LLM) Code Generation".
- Candidate 4 ("Generating Secure and High-Quality Code with LLMs by Combining CWE Vulnerability Knowledge and Business Logic Understanding") has no exact or near-exact match. It overlaps in concept with SecureCoder, whose method describes business logic and then retrieves CWE guidance. Reviewers may ask how we differ, so the paper should state explicitly that our "business rules" are developer-confirmed authorization, ownership and workflow constraints.

### Gaps
- I did not search dblp, Google Scholar, ACM DL or IEEE Xplore directly, so venue-only papers (not on arXiv) may be missed.
- Authors are unverified for Spec-Driven Development Benchmark (2606.00167), Constitutional SDD (2602.02584), SPARK, and most arXiv hits.
- The BaxBench title wording is taken from a search summary, not the primary page.
- The candidate titles are four out of many possible wordings, so exact-title checks for every variant were not exhaustive.

## Q2: Which phrases are crowded and which are distinctive?

### Takeaway
"Secure code generation" (with or without "LLMs") is heavily overused, and "security (and quality) of LLM-generated code" is crowded too. "CWE + business rules", "business-rule-aware/business-logic-aware secure generation", and "developer-confirmed rules" do not appear in any title found.

### Cited Findings
- More than 15 titles found contain "Secure Code Generation", e.g. SCGAgent, SoK: AI Secure Code Generation, RealSec-bench, SecRepoBench, SecureAgentBench, Rethinking the Evaluation of Secure Code Generation, BRACE, MA-CoT, Reflexion. — [SCGAgent](https://arxiv.org/abs/2506.07313); [SoK](https://arxiv.org/html/2606.25195v1); [RealSec](https://arxiv.org/abs/2601.22706); [MA-CoT](https://arxiv.org/html/2605.24300)
- "Security of/in LLM-generated code" studies: Kharma et al., "Security of LLM-generated Code: A Comparative Analysis", WildCode Revisited, and the Cross-Model Comparative Study. — [2502.01853](https://arxiv.org/abs/2502.01853); [2605.23091](https://arxiv.org/abs/2605.23091); [2512.04259](https://arxiv.org/pdf/2512.04259); [2607.20713](https://arxiv.org/html/2607.20713)
- "CWE" in titles: SecureCoder ("Common Weakness Enumeration"), CWEval. CWE as a method is common (BRACE, MA-CoT, SecGent, WA-0CoT), but "CWE knowledge" as an exact title phrase was not found. — [SecureCoder](https://ieeexplore.ieee.org/document/11197590/); [CWEval](https://arxiv.org/abs/2501.08200); [BRACE](https://arxiv.org/abs/2606.16244)
- "Business logic" in secure-code-generation titles: none found. It appears only in non-generation titles (Anota) or in a different sense (Compiled AI), and SecureCoder uses it inside its method. — [Anota](https://arxiv.org/pdf/2512.20705); [Compiled AI](https://arxiv.org/abs/2604.05150); [SecureCoder](https://ieeexplore.ieee.org/document/11197590/)
- Searches found no work that injects explicit business rules (e.g., owner-only access) into LLM generation and measures authorization bugs. — [search summary context; closest is BAC-Bench repair](https://arxiv.org/html/2610.34290)
- "Specification-driven" plus security is an emerging phrase, with two 2026 titles (Constitutional SDD; SDD Benchmark). "Security tests as executable specifications" is also taken. — [2602.02584](https://www.emergentmind.com/papers/2602.02584); [2606.00167](https://arxiv.org/pdf/2606.00167); [2608.09740](https://arxiv.org/abs/2608.09740)
- "Security-aware" is used in HardSecBench ("Security Awareness") and in the poster on risk scenarios. "Requirement-aware" is used by RECRL, which is not about security. — [HardSecBench](https://arxiv.org/html/2601.13864v2); [RECRL](https://arxiv.org/pdf/2605.00433)

### Inferences
- Crowded, so avoid as the main hook: "Secure Code Generation with LLMs", "Security (and Quality) of LLM-Generated Code", "Revisiting/Rethinking … secure code generation", "Spec-Driven/Specification-Driven secure generation", "Security tests as executable specifications".
- Distinctive and apparently unused: "CWE Knowledge and Business Rules" together; "Business-Rule-Aware" or "Business-Logic-Aware Secure Code Generation"; "developer-confirmed business rules"; "authorization/ownership/workflow rules" in a generation title; "regression protection across feature edits" (no title found, although Reflexion and SecTDD discuss regressions).
- Ranking by distinctiveness (inference): Candidate 1 ≈ Candidate 3 > Candidate 4 > Candidate 2. Candidate 1 puts the unique pairing "CWE Knowledge and Business Rules" first, which helps both search and novelty. Candidate 3 begins with the crowded "Secure Code Generation with LLMs" stem. Candidate 4 does not signal the replication study. Candidate 2 does not name CWE, and its first half nearly duplicates the base paper.
- Positioning risk: the Kharma/Mohaisen group already has three closely related titles (base paper, MA-CoT, the prompting-methods evaluation with 5 LLMs × 4 languages). Our related-work section should cite all three and make clear how our study differs.

### Gaps
- I could not confirm whether "Business-Logic-Aware" appears in non-indexed venues such as workshops or theses.
- I did not verify the exact final published titles for arXiv-only works, which may change at camera-ready.
