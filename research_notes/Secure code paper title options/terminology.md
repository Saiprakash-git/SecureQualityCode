# Title Terminology for an LLM Secure-Code Re-run Study and a Business-Rule/CWE Security Framework Proposal

Scope note: 19 tool calls were used. Sources are primary where possible (MITRE CWE, OWASP, ACM SIGSOFT Empirical Standards, arXiv abstract pages). Several arXiv IDs returned by search are dated 2026 (for example 2605.x to 2609.x). They match today's date (2026-10-27), but only 2609.23270 was opened directly; the others were seen only as search snippets.

## Q1. "Business logic" vs "business rules" vs "access control" vs "authorization" vs "application logic"

### Takeaway
MITRE itself defines "business logic" as problems that need "business rules" to classify. Its Business Logic Errors category (CWE-840) contains ownership (CWE-283, CWE-639, CWE-708) and workflow-order (CWE-841) weaknesses, while missing or incorrect role checks (CWE-862/863/285) sit under OWASP's "Broken Access Control". The most accurate plain title wording for ownership + role + workflow rules is therefore "business rules" for the input (what developers confirm), and "access-control and workflow" (or "business-logic") for the weakness classes. "Application logic" has no standing in the taxonomies and should be avoided.

### Cited Findings
- CWE-840 "Business Logic Errors" is a **Category**, and its mapping use is "PROHIBITED": "This CWE ID must not be used to map to real-world vulnerabilities." Its parents are View 699 and OWASP Top Ten 2021 A04 Insecure Design (CWE-1348). — [MITRE CWE-840](https://cwe.mitre.org/data/definitions/840.html)
- CWE-840 says the "Business Logic" term "is generally used to describe issues that require domain-specific knowledge or 'business rules'". It adds that many such issues "can be understood as instances of other weaknesses such as input validation, access control, numeric computation, order of operations, etc." — [MITRE CWE-840](https://cwe.mitre.org/data/definitions/840.html)
- CWE-840 members: 283 Unverified Ownership, 639 Authorization Bypass Through User-Controlled Key, 640 Weak Password Recovery, 708 Incorrect Ownership Assignment, 770 Allocation Without Limits, 826 Premature Release of Resource, 837 Improper Enforcement of a Single Unique Action, 841 Improper Enforcement of Behavioral Workflow. CWE-862 and CWE-863 are **not** members. — [MITRE CWE-840](https://cwe.mitre.org/data/definitions/840.html)
- CWE-841: the product "does not properly ensure that the actor performs the behaviors in the required sequence". Out-of-order or skipped steps let an attacker "manipulate the business logic". Notes: "typically associated with business logic flaws". Its parent is CWE-691 Insufficient Control Flow Management. — [MITRE CWE-841](https://cwe.mitre.org/data/definitions/841.html)
- OWASP Top 10 2021 A01 Broken Access Control: "Access control enforces policy such that users cannot act outside of their intended permissions." It covers IDOR ("viewing or editing someone else's account, by providing its unique identifier"), elevation of privilege, and missing API checks on POST/PUT/DELETE. It maps CWE-639, 862, 863 and 285, and is ranked #1, with 34 CWEs, 318,487 occurrences and 19,013 CVEs. — [OWASP Top 10 A01:2021](https://top10.owasp.org/2021/A01_2021-Broken_Access_Control)
- OWASP community page: business logic vulnerabilities are "ways of using the legitimate processing flow of an application" that produce "a negative consequence to the organization". The examples are workflow-type ("Product is given before transaction is verified"). It treats the category as distinct from broken controls and from authentication, and it lists CWE-285 as related. — [OWASP Business logic vulnerability](https://community.owasp.org/vulnerabilities/Business_logic_vulnerability)
- OWASP API Security Top 10 2023 separates API1 Broken Object Level Authorization (ownership/IDOR) from API6 "Unrestricted Access to Sensitive Business Flows" (abuse of legitimate workflows). — [ISACA review of 2023 OWASP API Top 10](https://www.isaca.org/resources/news-and-trends/industry-news/2023/reviewing-the-2023-owasp-api-top-10); [Noname BOLA explainer](https://nonamesecurity.com/learn-api-01-broken-object-level-authorization)
- The closest recent prior work on LLM code uses "access control" (and "Access Tasks" in its title), not "business logic" or "authorization". — [Dhuri, arXiv 2609.23270, "Specification Before Generation..."](https://arxiv.org/abs/2609.23270)
- Industry and thesis sources on LLM-generated code use "business logic flaws", "IDOR" and "BOLA" informally, and note that a clean build or static scan can hide such flaws. — [Univ. Turku thesis](https://www.utupub.fi/bitstreams/8ddd5d88-163b-450f-9b17-7010694d7c43/download); [Vidoc benchmark](https://blog.vidocsecurity.com/blog/detecting-complex-vulnerabilities-in-real-world-code-llm-benchmark-for-enhanced-software-security)

### Inferences
- Ranking for the ownership/role/workflow concept in a title:
  1. **"business rules"**. This is the most accurate word for the *input* (developer-confirmed who-owns-what, who-may-do-what, step order), and MITRE itself uses it. It is plain English and does not overclaim. Its weakness is that it is not itself a security keyword, so pair it with a security term.
  2. **"access-control and workflow" (flaws/rules)**. This is the most precise wording for the *weakness classes* and is easy to search: "access control" is the #1 OWASP category. It names role and ownership (via IDOR/BOLA) and workflow explicitly. It is longer.
  3. **"business-logic" (flaws/vulnerabilities)**. Reviewers recognise it and it covers all three rule types. However, it is formally a prohibited-for-mapping umbrella category, it excludes CWE-862/863, and in OWASP usage it leans toward "abuse of legitimate flows". It is acceptable in a title if the abstract pins it to the CWE list.
  4. **"authorization"**. It is accurate for role and ownership checks (CWE-285/862/863/639) but does not cover workflow order (CWE-841).
  5. **"application logic"**. It has no taxonomy anchor and is vague. Avoid it.
- Suggested phrasing: "...from CWE knowledge and business rules..." (input side), or "...for access-control and workflow flaws" (target side).

### Gaps
- No reliable search-volume data (Google Trends or Scholar counts) was retrieved for these phrases.
- The OWASP ASVS V8 (Authorization) and the business-logic chapter text were not fetched directly.

## Q2. "CWE knowledge" vs "vulnerability knowledge" vs "CWE-guided" vs "weakness-aware"

### Takeaway
Recent LLM secure-code papers commonly say "CWE-informed", "CWE-guided" and "security knowledge", and benchmarks are named after CWE (CWEval, CWEBench). "Weakness-aware" is rare. Because the framework draws specifically on the CWE catalogue, "CWE" in the title is both accurate and well indexed. "Vulnerability knowledge" is broader and slightly less precise: CWE catalogues *weaknesses*, while CVEs are *vulnerabilities*.

### Cited Findings
- A 2026 follow-up by Kharma's group introduces a "CWE-informed prompting strategy (WA-0CoT)". — [An Empirical Evaluation of LLM-Generated Code Security Across Prompting Methods, alphaXiv 2605.24298](https://www.alphaxiv.org/abs/2605.24298)
- MA-CoT "embeds task-specific CWE mitigation guidance". — [Enhancing Reliability in LLM-Based Secure Code Generation, arXiv 2605.24300](https://arxiv.org/html/2605.24300v1)
- BRACE uses the title wording "Activating Latent **Security Knowledge** through LLM-Guided Risk Analysis" and performs CWE risk analysis. — [arXiv 2606.16244](https://arxiv.org/html/2606.16244)
- The R+R replication's title says "with External **Security Knowledge**". — [arXiv 2605.31135](https://arxiv.org/pdf/2605.31135)
- "Guiding AI to Fix Its Own Flaws" reports that hints which name the target weakness and explain how it arises work better. — [arXiv 2506.23034](https://arxiv.org/abs/2506.23034)
- SafeGenBench links its questions to 44 CWE types, and CWEval/CWEBench are the standard secure-generation benchmarks. — [arXiv 2506.05692](https://arxiv.org/pdf/2506.05692); [Secure-Instruct arXiv 2510.07189](https://arxiv.org/pdf/2510.07189)

### Inferences
- Ranking: (1) **"CWE knowledge"** or **"CWE-based"**: accurate, short, searchable, and reviewers know CWE. (2) **"security knowledge"**: matches 2026 title trends (BRACE, R+R) but is vaguer. (3) **"CWE-guided"**: common, but "guided" suggests prompting and undersells the requirements step. (4) **"vulnerability knowledge"**: technically looser than CWE's "weakness". (5) **"weakness-aware"**: jargon-like and rare. Avoid it.
- Suggested pairing: "CWE Knowledge and Business Rules", which is parallel, plain and accurate.

### Gaps
- No frequency counts of each phrase in titles were obtained.

## Q3. Mechanism words that don't overclaim for an unimplemented design

### Takeaway
"Security requirements" is the plainest accurate word for what the framework produces, and it is a standard SE term. "Security tests" plus "executable specifications" is already used in a close 2026 paper (SecTDD). Words that assert guarantees ("secure-by-construction", "verified", "enforcement", "by design") would overclaim for a proposal. "Secure by Design" also carries a specific CISA policy meaning.

### Cited Findings
- The closest prior work is titled "Security Tests as Executable Specifications for LLM Code Generation: Benefits, Trade-offs, and Coverage Limits" (SecTDD). It uses tests before generation and as repair feedback, and finds that candidates which pass visible tests still fail hidden behaviour families. — [arXiv 2608.09740](https://arxiv.org/abs/2608.09740)
- "Specification Before Generation" uses "specification frame" and "pre-registered" in its title, and it backs its claims with a five-model evaluation. — [arXiv 2609.23270](https://arxiv.org/abs/2609.23270)
- Other "specification" titles: "DualGauge: ... Specification-Only Code Generation" and "Specification-Driven Development Benchmark: Security Knowledge Transition". — [arXiv 2511.20709](https://arxiv.org/pdf/2511.20709); [arXiv 2606.00167](https://arxiv.org/pdf/2606.00167)
- "Secure by Design" is a CISA-led joint government initiative (with the FBI, NSA and international partners, 2023) about manufacturers taking ownership of security outcomes and secure defaults. It is a policy term, not a technical guarantee. — [FedScoop](https://fedscoop.com/cisa-and-partners-issue-secure-by-design-principles-for-software-manufacturers/); [Axios](https://www.axios.com/2023/10/18/cisa-cyber-security-secure-by-design-principles)

### Inferences
- Ranking for a design proposal (accuracy / clarity / overclaim risk):
  - **"security requirements"**: high, high, low. It is the best core noun. "Code-linked security requirements" or "security requirements tied to code" conveys the location-binding without jargon.
  - **"requirement-driven"** / **"requirements-first"**: high, high, low. It conveys "before generation".
  - **"security tests"** (and "tests that catch missing defenses"): high, high, low. This is plainer than "mutation-validated" and differentiates the work from SecTDD, because the tests are themselves checked for sensitivity.
  - **"traceable"**: accurate (requirement to code location to test), and a known SE term. Medium clarity, low risk.
  - **"specification-guided"**: accurate but crowded (SecTDD, Dhuri, DualGauge), and "specification" may suggest formal specs.
  - **"security contracts"**: implies formal design-by-contract or runtime enforcement. Medium risk.
  - **"verified security checks"**: "verified" implies formal verification or demonstrated results. High risk.
  - **"enforcement"**: implies runtime guarantees. Medium-high risk.
  - **"by design"** / **"Secure-by-Design"**: collides with the CISA policy term and implies a guarantee. Avoid.
  - **"secure-by-construction"**: implies a formal correctness guarantee. Avoid for an unimplemented proposal.
- Honesty markers for the proposal half: "Toward(s)...", "A Framework Proposal for...", "Proposing...", "A Design for...". "Towards" is the conventional SE/security title marker for unevaluated designs.

### Gaps
- No formal survey of reviewer perceptions of "towards" was found. That claim rests on convention, not a source.

## Q4. Words for the empirical part (partial re-run, different prompt/temperature/scanners)

### Takeaway
Under the ACM SIGSOFT Empirical Standards, the work is a **partial, external, conceptual (or methodological) replication**, because the method details (prompt, temperature, scanners) and context (models) differ. "Replication" alone may read as an exact repeat. "Revisiting" or "re-evaluation" is the plain, low-risk title wording, and the abstract should then state the SIGSOFT type. 2026 titles combine both ("Revisited ... Empirical Study", "R+R: Reassessing ... A Replication").

### Cited Findings
- SIGSOFT definition: a replication "deliberately repeats a previous study ... to determine whether its results can be reproduced". **Reproduce** means re-analysing the original data. **Replicate** means collecting new data and analysing it the original way. — [ACM SIGSOFT Empirical Standards, Replication](https://www2.sigsoft.org/EmpiricalStandards/docs/standards?standard=Replication)
- By similarity: **Exact** (same RQs, method, context), **Methodological** (same RQs and method, different context), **Conceptual** (same RQs, different method, different context). By overlap: **partial / complete / extended**. By participants: **internal / external / mixed**. — [same](https://www2.sigsoft.org/EmpiricalStandards/docs/standards?standard=Replication)
- Essential attributes include that the work "describes and justifies any differences from the original study" and clearly separates results that are "consistent and inconsistent with the original study". — [same](https://www2.sigsoft.org/EmpiricalStandards/docs/standards?standard=Replication)
- SIGSOFT lists "Rejecting a study because it replicates or reproduces existing work" as an invalid criticism. — [ACM SIGSOFT Empirical Standards](https://www2.sigsoft.org/EmpiricalStandards/)
- ACM swapped the meanings of "replicated" and "reproduced" in its badging terms to align with NISO, so the terms are confusable. — [ACM badging terms](https://www.acm.org/publications/badging-terms)
- Title precedents: "WildCode Revisited: A Comprehensive Empirical Study on the Security of LLM-Generated Code"; "R+R: Reassessing Java Security API Misuse in Current LLMs: A Replication on JCA and JSSE APIs ..."; "Guiding AI to Fix Its Own Flaws: An Empirical Study on LLM-Driven Secure Code Generation". — [arXiv 2512.04259](https://arxiv.org/html/2512.04259); [arXiv 2605.31135](https://arxiv.org/pdf/2605.31135); [arXiv 2506.23034](https://arxiv.org/abs/2506.23034)
- The original paper is Kharma, Choi, Alkhanafseh and Mohaisen, "Security and Quality in LLM-Generated Code: A Multi-Language, Multi-Model Analysis" (arXiv 2502.01853). It used 200 tasks and Python/Java/C++/C. The search results could not independently confirm the TDSC venue (the v2 PDF shows a TDSC header). — [arXiv 2502.01853](https://arxiv.org/html/2502.01853v1)

### Inferences
- Ranking: (1) **"Revisiting"** or **"...Revisited"**: accurate for a partial re-run, plain, and it signals a link to prior work without claiming exactness. (2) **"Re-evaluation"** / **"Reassessing"**: accurate and plain. (3) **"Empirical study"**: accurate, generic and well indexed, and works well as a subtitle. (4) **"Replication"**: accurate only with a qualifier ("a partial conceptual replication"). Bare, it invites "why not exact?" critiques. (5) **"Assessment"**: fine but bland. (6) **"Benchmark"**: inaccurate, because no new benchmark is introduced (the original's tasks are reused). Avoid it.
- In the abstract or method, state: "a partial, external, conceptual replication (ACM SIGSOFT Empirical Standards) of Kharma et al." Also list the deviations (prompt, temperature, scanners, models).

### Gaps
- It is unclear whether the user's study answers all of the original RQs ("partial" vs "complete") and whether the prompt change counts as a different *method* (conceptual) or the same method in a new context (methodological). The authors must decide.

## Q5. Model descriptors: "recent", "current", "cost-efficient", "low-cost", "budget"

### Takeaway
Recent titles use "current LLMs" (R+R) or name model classes such as "open-weight". No titles were found that frame code-security studies around "low-cost" or "cost-efficient" models, so the cost angle is a possible differentiator, but it needs a stated criterion (a price per million tokens). "Budget" sounds informal.

### Cited Findings
- "Reassessing Java Security API Misuse in **Current LLMs**". — [arXiv 2605.31135](https://arxiv.org/pdf/2605.31135)
- "open-weight coding models" appears in a 2025 security-trend paper, and CoV-Eval evaluates "20 proprietary and open-source models". Neither frames its models by cost. — [arXiv 2511.04538](https://arxiv.org/pdf/2511.04538v1); [CoV-Eval, ACL 2025](https://aclanthology.org/2025.acl-long.849)
- A search for "low-cost", "cost-efficient" or "open-weight" in code-security titles found no matching titles. — (negative search result; see the Gaps entry)

### Inferences
- Ranking: (1) **"lower-cost"** / **"low-cost"**: accurate if the paper defines it (for example below $X per million tokens), plain, and differentiating. "Lower-cost" is safer because it is relative to the original's flagship models. (2) **"recent"**: accurate but dates quickly and carries little information. Pair it: "recent lower-cost LLMs". (3) **"current"**: same issue as "recent". (4) **"cost-efficient"**: implies a measured quality-per-dollar result, which is acceptable only because cost *is* measured. (5) **"budget"**: informal. Avoid it.
- Because cost is measured, "low-cost" plus a measured cost metric is defensible. "Cost-efficient" would claim the conclusion in the title.

### Gaps
- The absence of "low-cost" titles is based on two searches only, not a systematic arXiv title query.

## Q6. Most searched/indexed keywords (IEEE / ACM CCS)

### Takeaway
No IEEE thesaurus or ACM CCS tree was retrieved directly. From the sources above, the high-value indexed strings are "large language models", "code generation", "secure code generation", "CWE", "access control", "business logic", "security requirements", "security testing", "static analysis", "empirical study" and "replication".

### Cited Findings
- The ACM CCS has a top-level "Security and privacy" category with the subtree "Software and application security" (seen in an archived 2021 snapshot). — [ACM CCS archived page](https://webcf.waybackmachine.org/web/20210531060817/https://dl.acm.org/ccs)
- Recurrent title vocabulary in 2025–2026 papers: "Secure Code Generation" (Reflexion 2511.03898, BRACE 2606.16244, DeepGuard 2604.09089), "LLM-Generated Code" (WildCode, Kharma), "Empirical Study", "Specification(s)", "Security Knowledge". — [arXiv 2511.03898](https://arxiv.org/html/2511.03898v1); [arXiv 2606.16244](https://arxiv.org/html/2606.16244); [arXiv 2604.09089](https://arxiv.org/pdf/2604.09089); [arXiv 2512.04259](https://arxiv.org/html/2512.04259)

### Inferences
- Likely ACM CCS concepts (unverified, from training knowledge): Security and privacy → Software and application security → Software security engineering; Security and privacy → Security services → Access control; Software and its engineering → Software testing and debugging; Computing methodologies → Natural language processing (or Machine learning). These must be checked at dl.acm.org/ccs before use.
- Likely IEEE keyword choices (unverified): "Large language models", "Codes", "Security", "Software testing", "Access control", "Static analysis", "Requirements engineering". Check them against the IEEE Thesaurus in the submission system.
- Title keyword priorities: "LLM-Generated Code" or "Secure Code Generation" (core), plus "CWE" and "Business Rules"/"Access Control" (differentiators), plus "Revisiting"/"Empirical" (type).

### Gaps
- The IEEE Taxonomy/Thesaurus and the ACM CCS 2012 tree could not be fetched.
- No Google Trends or Scholar hit counts were obtained.
- Exact placement of the CCS concepts is unconfirmed.
