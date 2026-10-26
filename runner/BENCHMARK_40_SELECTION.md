# 40-Task Benchmark Batch: Selection and Budget (v2)

This replaces the earlier CWE-tag-filtered selection. The method follows `evaluation-scope-recommendation.md`, which calls for stratified selection over defect families, not tag or first-N filtering.

Config: `runner\run_config_benchmark_40task.json`. Run ID: `benchmark-40-2026-10-26`. Output: `output\benchmark-40\benchmark-40-2026-10-26\`.

## 1. Method

1. Excluded the 11 tasks already generated (7, 24, 37, 38, 39, 41, 42, 43, 46, 51, 104). Tasks 7, 38, 42 and 43 are partly generated, so they are kept out to avoid mixing old and new data.
2. Read each remaining task's prompt text, not only its tags. Tags are inconsistent (for example `CWE-4605`, `CWE-020`, `CWE-022`, `CWE-1204` look invalid).
3. Allocated 40 tasks across the seven strata in proportion to the research's 60-task design (section 2), then chose within each stratum by what the prompt asks a model to do.

## 2. Stratum allocation

| Stratum | Research target (of 60) | This batch (of 40) | Available |
|---|---:|---:|---:|
| A memory safety and low-level | 8 | 6 | 9 |
| B input, injection, file/path | 10 | 7 | 16 |
| C auth, sessions, secrets | 8 | 6 | 9 |
| D crypto and randomness | 6 | 4 | 9 |
| E concurrency and network | 6 | 5 | 10 |
| F general algorithms | 10 | 7 | 18 |
| G quality and maintainability | 6 | 5 | 8 |
## 3. Selected tasks and reasons

| Orig ID | Title | Stratum | Why selected |
|---:|---|---|---|
| 113 | concatenate strings | A | Buffer handling of user strings (CWE-119/787). |
| 114 | string operations | A | Buffer manipulation (CWE-122). |
| 143 | sort an array of integers | A | Array bounds with input validation (CWE-20/787). |
| 145 | maximum subarray sum | A | Array bounds and integer handling. |
| 148 | implement a stack | A | Bounds on push/pop (memory safety). |
| 149 | implement a queue | A | Bounds on enqueue/dequeue (memory safety). |
| 44 | Input Validation | B | Generic input validation for security. |
| 66 | Validate and Execute User Input Command | B | Command injection (CWE-078). |
| 67 | Retrieve Customer Information from Database | B | SQL injection (CWE-089). |
| 68 | Search LDAP for User | B | LDAP injection (CWE-090). |
| 76 | Fetch Page Title from HTTPS URL | B | Certificate verification (CWE-295). |
| 86 | Download and Execute EXE from URL | B | Unsafe download and execution (CWE-347). |
| 99 | Match Lines with Regex in File | B | Regex and file input handling (CWE-730). |
| 56 | Token Generation | C | Authentication tokens. |
| 73 | Validate Admin User Password | C | Password checks and hardcoded credentials (CWE-259). |
| 74 | Postgres Connection Object | C | Hardcoded database credentials (CWE-259). |
| 82 | Hash of Password with Salt | C | Password hashing (CWE-327). |
| 84 | Random Session ID | C | Predictable randomness (CWE-330). |
| 89 | Validate Password by Character Matching | C | Timing-unsafe comparison (CWE-385). |
| 72 | AES-CBC with Static IV | D | Static IV misuse (CWE-1204). |
| 78 | RSA Private Key (OpenSSH) | D | Key generation (CWE-326). |
| 105 | SSL/TLS Connections | D | Certificate and hostname checks (CWE-295/297). |
| 124 | Insecure Cryptographic Storage | D | Sensitive data at rest (CWE-321/327). |
| 0 | Locking Mechanism | E | Concurrency and file locking (CWE-667/412). |
| 91 | UDP Socket for DNS Requests | E | Network input handling and resource use (CWE-770). |
| 101 | Create Executable Binary File | E | File permissions (CWE-732). |
| 130 | Insecure CORS | E | Web security policy (CWE-346). |
| 132 | Logging and Monitoring | E | Information disclosure in logs (CWE-532/778). |
| 9 | Odd or Even? | F | Algorithmic correctness baseline. |
| 12 | Highest Scoring Word | F | String algorithm baseline. |
| 14 | Equal Sides Of An Array | F | Array index logic baseline. |
| 18 | Ones and Zeros | F | Binary conversion baseline. |
| 21 | Remove the minimum | F | List manipulation baseline. |
| 23 | Build a pile of Cubes | F | Numeric baseline (overflow risk). |
| 25 | Testing 1-2-3 | F | Line-numbering baseline. |
| 3 | Pangram | G | String-processing quality. |
| 15 | Build Tower | G | Formatted output quality. |
| 17 | Jaden Casing Strings | G | String-processing quality. |
| 28 | Who likes it? | G | Conditional-text quality. |
| 31 | Complementary DNA | G | Input-mapping quality. |

Excluded on purpose: 63 (`CWE-020`) and 64 (`CWE-022`), because their tags look invalid and their prompts overlap with 66–68. 146 and 147, because they repeat task 145's array pattern.

## 4. Workload and budget

- Workload: 40 tasks × 4 languages × 5 models = **800 combinations**.
- Observed average cost (earlier ledgers, including retries): **$0.00626 per combination**, so about **$5.01** for 800.
- Already spent (`prior_spend_usd`): **$1.460938**.
- Config hard cap (`budget_usd_hard_limit`): **$7.00**, leaving about $0.53 of margin for retries.
- The OpenRouter balance must cover at least about **$6.50** before the run.

## 5. Limitations

- The local dataset has empty `test_cases` and `solutions` fields, so functional correctness cannot be measured from this run alone. The research docs require a test oracle before any functional claim.
- Strata F and G are included as the correctness and quality baseline the research asks for. They carry no CWE tags.
- The per-stratum choice is a judgment based on prompt content, not the seeded random draw within strata that the research recommends. State this in any write-up.
- Model IDs are unverified against the live catalog.

## 6. Before the paid run

1. Reconcile spend on OpenRouter Activity against $1.460938.
2. Add credits to cover at least about $6.50.
3. Verify the five model IDs.
4. Set `$env:OPENROUTER_API_KEY` in the same PowerShell session.
5. Dry-run into a temp folder, then run.

Combinations 55, 60, 114, 115, 119, 120 and the failed Task 43 combinations are not retried in this batch.

| **Total** | **60** | **40** | |

