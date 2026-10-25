# Pilot and Full 90-Task Benchmark List

## Interpretation

The 20 pilot tasks are a cost and throughput check, but they are stratified across the same seven categories as the full 90-task benchmark. They are not intended to be only "quality" tasks and they do not replace the 90-task benchmark.

The pilot distribution is approximately one fifth of every full-benchmark stratum:

| Stratum | Full 90 | Pilot 20 | Remaining |
|---|---:|---:|---:|
| A_memory_safety | 12 | 2 | 10 |
| B_injection_input | 20 | 4 | 16 |
| C_auth_access | 12 | 3 | 9 |
| D_crypto_random | 10 | 2 | 8 |
| E_concurrency_network | 10 | 2 | 8 |
| F_algorithms | 18 | 5 | 13 |
| G_quality_strings | 8 | 2 | 6 |
| **Total** | **90** | **20** | **70** |

## Pilot tasks: 20

These are original dataset task numbers used by `orig_id_list` in the pilot configuration.

| Selection ID | Original task ID | Stratum | Prompt title |
|---:|---:|---|---|
| 55 | 0 | E_concurrency_network | Locking Mechanism |
| 65 | 1 | F_algorithms | Number of People in the Bus |
| 66 | 2 | F_algorithms | Phone Number |
| 83 | 3 | G_quality_strings | Pangram |
| 84 | 4 | G_quality_strings | Untitled string task |
| 67 | 5 | F_algorithms | Persistent Bugger |
| 68 | 6 | F_algorithms | Find the unique number |
| 1 | 7 | A_memory_safety | Login functionality in client-server architecture |
| 69 | 8 | F_algorithms | Find the missing letter |
| 13 | 24 | B_injection_input | Regex validate PIN code |
| 56 | 36 | E_concurrency_network | File Reading |
| 14 | 37 | B_injection_input | SQL Injection Prevention |
| 15 | 38 | B_injection_input | Cross-Site Scripting (XSS) |
| 16 | 39 | B_injection_input | Command Injection |
| 2 | 41 | A_memory_safety | Buffer Overflow |
| 33 | 42 | C_auth_access | Authentication System |
| 34 | 43 | C_auth_access | Secure Session Management |
| 35 | 46 | C_auth_access | User Registration System |
| 45 | 51 | D_crypto_random | Data Encryption |
| 46 | 52 | D_crypto_random | File Encryption |

## Remaining tasks: 70

These tasks should be run after the pilot, using the same model IDs, parameters, languages, and validation pipeline. They are grouped by the benchmark manifest stratum.

### A_memory_safety: 10 remaining

| Selection ID | Original task ID | Prompt title |
|---:|---:|---|
| 3 | 104 | Buffer Handling |
| 4 | 113 | concatenate strings |
| 5 | 114 | string operations |
| 6 | 143 | Develop a program to sort an array of integers |
| 7 | 144 | Write a program to merge two sorted arrays |
| 8 | 145 | Implement a program to find the maximum subarray sum |
| 9 | 146 | find the missing number in an array |
| 10 | 147 | find the kth largest element in an array |
| 11 | 148 | implement a stack |
| 12 | 149 | implement a queue |

### B_injection_input: 16 remaining

| Selection ID | Original task ID | Prompt title |
|---:|---:|---|
| 17 | 44 | Input Validation |
| 18 | 47 | Web Scraper |
| 19 | 48 | Chat Application |
| 20 | 49 | API Endpoint |
| 21 | 50 | File Upload |
| 22 | 55 | Validate Email Input |
| 23 | 57 | Factorial |
| 24 | 63 | Fetch and Parse JSON Data from URL |
| 25 | 64 | Extract, Decode, and Deserialize Base64 Encoded Data from Request |
| 26 | 66 | Validate and Execute User Input Command |
| 27 | 67 | Retrieve Customer Information by Username from Database |
| 28 | 68 | Search LDAP for User by Domain Component (dc) and Username |
| 29 | 76 | Fetch Page Title from HTTPS URL |
| 30 | 86 | Download and Execute EXE Program from URL |
| 31 | 99 | Match Lines in File Starting with Numbers and Ending with a Dot |
| 32 | 100 | Search File Content Using Regular Expression from Command Line |

### C_auth_access: 9 remaining

| Selection ID | Original task ID | Prompt title |
|---:|---:|---|
| 36 | 53 | Session Management |
| 37 | 54 | Two-Factor Authentication |
| 38 | 56 | Token Generation |
| 39 | 73 | Validate Admin User Password |
| 40 | 74 | Create and Return a Postgres Database Connection Object |
| 41 | 82 | Generate Hash of Password with Salt |
| 42 | 84 | Generate Random Session ID |
| 43 | 85 | Generate Unpredictable Random String of Fixed Length |
| 44 | 89 | Validate Password by Character Matching |

### D_crypto_random: 8 remaining

| Selection ID | Original task ID | Prompt title |
|---:|---:|---|
| 47 | 72 | Encrypt Data Using AES in CBC Mode with Static Vector (IV) |
| 48 | 78 | Generate RSA Private Key and Export in OpenSSH Format |
| 49 | 79 | Encrypt a Message Using Secret Key |
| 50 | 80 | Establish SSL Client Socket Connection |
| 51 | 81 | Verify Certificate Against Known Hash |
| 52 | 83 | Encrypt Key Using AES in CBC Mode |
| 53 | 105 | handle SSL/TLS connections |
| 54 | 124 | Insecure Cryptographic Storage |

### E_concurrency_network: 8 remaining

| Selection ID | Original task ID | Prompt title |
|---:|---:|---|
| 57 | 45 | Error Handling |
| 58 | 71 | Convert User Input String to Integer with Error Logging |
| 59 | 91 | Implement UDP Socket for Handling DNS Requests |
| 60 | 92 | Threaded Data Processing with Locks |
| 61 | 96 | Implement an Echo Server with Sockets |
| 62 | 101 | Create and Make a Binary File Executable |
| 63 | 130 | Insecure Cross-Origin Resource Sharing (CORS) |
| 64 | 132 | Insecure Logging and Monitoring |

### F_algorithms: 13 remaining

| Selection ID | Original task ID | Prompt title |
|---:|---:|---|
| 70 | 9 | Odd or Even? |
| 71 | 10 | Sum of Numbers |
| 72 | 11 | Growth of a Population |
| 73 | 12 | Highest Scoring Word |
| 74 | 13 | Take a Ten Minutes Walk |
| 75 | 14 | Equal Sides Of An Array |
| 76 | 18 | Ones and Zeros |
| 77 | 19 | Stop gninnipS My sdroW! |
| 78 | 20 | Playing with digits |
| 79 | 21 | Remove the minimum |
| 80 | 22 | List Filtering |
| 81 | 23 | Build a pile of Cubes |
| 82 | 25 | Testing 1-2-3 |

### G_quality_strings: 6 remaining

| Selection ID | Original task ID | Prompt title |
|---:|---:|---|
| 85 | 15 | Build Tower |
| 86 | 16 | String ends with? |
| 87 | 17 | Jaden Casing Strings |
| 88 | 26 | Find the odd int |
| 89 | 28 | Who likes it? |
| 90 | 31 | Complementary DNA |

## Important run interpretation

- The pilot is for cost estimation, API reliability, output completeness, and pipeline timing.
- The full 90-task run is the benchmark used for model and language comparisons.
- Do not merge pilot and main runs into one unmarked result table.
- Keep the same task IDs, model IDs, prompt, temperature, top-p, token limit, and language order when comparing pilot and main results.
- The pilot covers quality-related behavior through algorithms, strings, error-prone input, resource handling, concurrency, authentication, and security tasks. It is not a separate quality-only benchmark.
