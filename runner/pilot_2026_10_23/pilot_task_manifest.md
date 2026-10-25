# Pilot Task Manifest

This isolated pilot uses 20 original dataset task IDs. The runner will evaluate every task with every configured model and request all four languages.

## Workload

```text
20 tasks x 5 models = 100 API requests
100 responses x 4 languages = 400 expected source files
```

## Selected tasks

| Original ID | Title | Dataset tags |
|---:|---|---|
| 0 | Locking Mechanism | concurrency, file locking, CWE-667, CWE-412, CWE-820 |
| 1 | Number of People in the Bus | algorithms |
| 2 | Phone Number | arrays, strings, regular expressions, algorithms |
| 3 | Pangram | none recorded |
| 4 | Untitled string task | strings |
| 5 | Persistent Bugger | mathematics |
| 6 | Find the unique number | algorithms, arrays |
| 7 | Login functionality in client-server architecture | buffer overflow, input validation |
| 8 | Find the missing letter | algorithms, mathematics |
| 24 | Regex validate PIN code | regular expressions |
| 36 | File Reading | file handling, resource management |
| 37 | SQL Injection Prevention | SQL injection, database security |
| 38 | Cross-Site Scripting (XSS) | XSS, web security |
| 39 | Command Injection | command injection, system security |
| 41 | Buffer Overflow | buffer overflow, memory safety |
| 42 | Authentication System | authentication, password security |
| 43 | Secure Session Management | session management, web security |
| 46 | User Registration System | user registration, data storage, password security |
| 51 | Data Encryption | encryption, data security |
| 52 | File Encryption | encryption, file security |

## Important interpretation

The IDs above are original dataset `task_number` values. They are not the stable `selection_id` values used by the 90-task planning manifest. The new isolated JSON configuration uses `orig_id_list`, so these are the IDs the runner will load.
