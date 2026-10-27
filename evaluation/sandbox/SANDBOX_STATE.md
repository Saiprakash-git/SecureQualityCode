# eval-sandbox: installed state (2026-10-26)

Set up following `functional/SANDBOX_PROPOSAL.md` (steps 1–3, approved). No optional packages were installed: no dev headers and no Python packages.

## Installed

| item | version / location |
|---|---|
| WSL | 2.6.1.0, kernel 6.6.87.2-microsoft-standard-WSL2 (already present) |
| Distro | `eval-sandbox`, Ubuntu 24.04.5 LTS, VHDX at `D:\wsl\eval-sandbox\ext4.vhdx` |
| gcc / g++ | 13.3.0-6ubuntu2~24.04.1 |
| libc6-dev | 2.39-0ubuntu8.9 |
| python3.12 | 3.12.3-1ubuntu0.16 |
| openjdk-21-jdk-headless | 21.0.12.1+1-1~24.04.4 |
| bubblewrap | 0.9.0-1ubuntu0.3 |
| util-linux (prlimit) | 2.39.3-9ubuntu6.6 |
| coreutils (timeout) | 9.4-3ubuntu6.3 |
| tar | 1.35+dfsg-3ubuntu0.4 |
| JUnit console | junit-platform-console-standalone 1.11.4. SHA-1 matched Maven Central. SHA-256 `b016ef6b1c3454d6d7c2c88ce081dabf289699686af6622d6e4e2e1b54b4a2fc`. At `/opt/junit` (root, read-only). Host copy in `evaluation/.toolchain/junit/`. |

Packages were installed with `apt-get install --no-install-recommends gcc g++ libc6-dev python3 openjdk-21-jdk-headless bubblewrap util-linux coreutils tar ca-certificates`.

## Hardening (`/etc/wsl.conf`)

- `systemd=false`
- `[automount] enabled=false, mountFsTab=false`
- `[interop] enabled=false, appendWindowsPath=false`
- `[user] default=evalrunner`
- `[boot] command` unregisters the `WSLInterop` and `WSLInterop-late` binfmt handlers on every start. `[interop] enabled=false` alone left `WSLInterop-late` registered: a Windows exe was handed to `/init`, which tried the host over vsock. With the handlers removed, the kernel reports `Exec format error`.
- User `evalrunner` (uid 1000): password locked, no sudo.
- The sandbox scripts live in `/opt/eval-sandbox` (root-owned):
  - `run_isolated.sh` (SHA-256 `67d4fdd8…`)
  - `receive_and_run.sh` (`d0c13f8c…`)
  - `seccomp.bpf` (`151e7aa5…`, from `make_seccomp.py`)
  - `probe.exe` (a copy of `whoami.exe`, used only by the Windows-exec check)

Not changed: `%USERPROFILE%\.wslconfig`. WSL's default VM memory cap is 7.6 GB, half of RAM.

## Per-test sandbox (`run_isolated.sh`)

- **Namespaces and privileges:** bubblewrap `--unshare-all` (user, PID, IPC, UTS, cgroup, network: loopback only), `--cap-drop ALL`, no_new_privs.
- **Syscall filter:** seccomp makes `socket(AF_VSOCK)` fail with EAFNOSUPPORT, and kills on a foreign architecture or an x32 syscall.
- **Filesystem:** the root is read-only. `/usr` and `/opt/junit` are read-only. `/usr/lib/wsl` is hidden, which also hides the 9p view of the Windows driver store. The input is read-only at `/src`. `/ws` (256 MB) and `/tmp` (64 MB) are tmpfs. `/dev` is minimal and `/proc` private.
- **Environment and I/O:** environment cleared, stdin closed.
- **Limits:**
  - wall clock: `timeout -k 5`;
  - CPU seconds, address space and file size: `prlimit`, outside the sandbox;
  - process count: `prlimit --nproc`, inside, after the namespaces exist. Setting it before bubblewrap makes the kernel refuse the user namespace (EAGAIN).
  - The output file is capped by the file-size limit.

## Verification

- `check_hardening.sh`: 9/9 PASS (31 checks in total with `check_bwrap.sh`).
- `check_bwrap.sh`: 22/22 PASS on every run of the final script (3 direct runs, plus the executor self-test each time `WslBwrapExecutor` starts). Earlier versions failed; the history is in work_log.md step 11. Every check requires proof that the probe ran inside the sandbox, so a sandbox that fails to start fails every check.
  - Covered: identity and capabilities; network (TCP, DNS, interfaces); vsock; filesystem (6 write targets, home, `/mnt`, `/etc/shadow`, WSL drivers, `/init`, `/dev`); tmpfs caps; environment; PID visibility and signalling; memory; fork bomb; file size; output cap; private `/tmp`; wall and CPU timeouts; no surviving processes; Windows exe; toolchain controls (gcc, g++, javac/java, JUnit, python).
- **Negative control:** the same battery run through `unsafe_runner_for_selftest.sh` (no isolation). All isolation checks fail. The only passes are the two timeout checks (that runner also uses `timeout`), `windows_exe_blocked` (interop is off for the whole distro), and the JUnit and Python toolchain controls. This shows the battery detects an unsafe runner.
- `tests/test_wsl_sandbox.py`: 9/9. The executor runs the trusted fixtures through the sandbox and gets the expected outcome in every case:
  - Python: passed, failed, error ×2, timeout;
  - C: passed, failed, error, PASS/FAIL style, reviewed rename;
  - Java/JUnit: passed, failed.
  It refuses generated code without human approvals, tampered workspaces, unlisted fixtures, and an unverified sandbox.

## Residual risks

- WSL distros share one VM kernel. A kernel or WSL escape bug would bypass the sandbox. This is accepted as low risk; there are no secrets inside the distro.
- Result integrity is not protected against an adversarial artifact. Code under test shares a process with its test framework, so it could fake "OK" output or report files. The sandbox protects the machine, not the score. This is acceptable for LLM outputs on benign tasks, but suspicious artifacts should be spot-checked.
- Gate 1 compiled C/C++ with clang 20 (zig). The sandbox builds with gcc 13. A combination that passed gate 1 can still fail its gate 2 build; that is reported as `error: build_failed`, never as `failed`.
- No packages are installed for tests that need services or third-party Python libraries. Those combinations will end as `error` (import or link failure) unless their need is established and the packages are approved.
