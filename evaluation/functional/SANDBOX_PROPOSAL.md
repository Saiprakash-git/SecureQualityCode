# Linux sandbox for functional tests: proposal (not installed)

Status: **steps 1–3 approved and installed on 2026-10-26; see `../sandbox/SANDBOX_STATE.md` for the installed state and verification.** Optional packages (step 3 dev headers, Python packages) are not installed.

## Why a sandbox is needed

Gate 2 has to run generated code: the oracle tests call into it. That code is untrusted. Some of it opens sockets, spawns processes (`sys/wait.h`, `subprocess`), and touches files. Also, the C/C++ code targets POSIX/Linux. So the tests must run in an isolated Linux environment, with no network, no access to the host filesystem, and limits on time, memory and processes.

## Facts about this machine (checked read-only, 2026-10-26)

| item | state |
|---|---|
| OS | Windows 11 Home Single Language 10.0.26200 |
| WSL | 2.6.1.0, kernel 6.6.87.2-1, default version 2, **no distribution installed** |
| Virtualization | firmware-enabled, hypervisor present |
| Windows Sandbox / Hyper-V Manager | not available on Home edition |
| Docker / Podman | not installed |
| CPU / RAM | Ryzen 7 4800H, 16 threads / 16 GB |
| Free disk | C: 6.4 GB, F: (project) 6.2 GB, **D: 36.5 GB**, E: 19 GB (all NTFS) |

## Options considered

| option | fit | why |
|---|---|---|
| **WSL2 dedicated distro + bubblewrap per test (recommended)** | good | WSL2 is already installed. A separate distro is its own VM-backed filesystem. bubblewrap adds per-run isolation of network, PID, IPC, filesystem and user. No new Windows component. |
| Docker Desktop | poor | Needs WSL2 anyway. Several GB more disk, a background service, and license terms for larger organisations. Adds nothing a distro plus bubblewrap cannot do for this workload. |
| Podman/Docker engine inside the WSL distro | acceptable alternative | Stronger, image-based reproducibility, but more setup and disk. Can be added later on top of the recommended option. |
| Windows Sandbox | unavailable | Not on Home edition, and it is Windows rather than Linux. |
| VirtualBox/VMware VM | poor | Large install, can conflict with the Hyper-V platform WSL uses, and needs more disk than is free. |
| Remote CI runner (e.g. GitHub Actions) | not recommended | Sends code and tests off the machine, and the free tier has time limits. |

## Recommended setup (for approval)

1. **Dedicated distro on D:** Ubuntu 24.04, imported or installed as `eval-sandbox` with its virtual disk under `D:\wsl\eval-sandbox`, so C:/F: free space is untouched. Check the exact flags with `wsl --help` first (`wsl --install Ubuntu-24.04 --name eval-sandbox --location D:\wsl\eval-sandbox --no-launch` on WSL ≥ 2.4, or `wsl --import`). Estimated disk: about 3–4 GB including toolchains.
2. **Harden the distro** in `/etc/wsl.conf`:
   - `[automount] enabled=false`, so Windows drives are not mounted at `/mnt/c`, `/mnt/f`, ...
   - `[interop] enabled=false, appendWindowsPath=false`, so Linux processes cannot launch Windows executables.
   - Add a non-root user `evalrunner`, and leave no passwordless sudo for that user once setup is done.
   - Then `wsl --terminate eval-sandbox` to apply.
3. **Toolchain inside the distro**, with versions recorded in the results manifest: gcc/g++ (Ubuntu 24.04: GCC 13), python3 (3.12), openjdk-21-jdk-headless, bubblewrap, util-linux (prlimit), coreutils (timeout). The JUnit 5 `junit-platform-console-standalone` jar comes at a pinned version with its SHA-256 checked. Optional: `libsqlite3-dev libssl-dev libcurl4-openssl-dev libldap-dev libpq-dev`, which would also unblock the 32 gate-1 `blocked_dependency` artifacts. Approve those separately.
4. **Per-test isolation** (the `WslBwrapExecutor`, to be written after approval):
   - Copy the workspace in through a `tar` stream on stdin (`wsl -d eval-sandbox -u evalrunner -- ...`). No shared folder is involved, because automount is off.
   - Each step runs as `timeout -k 5 <limit> prlimit --as=<mem> --nproc=<n> --fsize=<n> --cpu=<s> bwrap --unshare-all --die-with-parent --new-session --ro-bind /usr /usr --symlink usr/lib /lib --symlink usr/lib64 /lib64 --symlink usr/bin /bin --proc /proc --dev /dev --tmpfs /tmp --bind <ws> /ws --chdir /ws <cmd>`.
   - `--unshare-all` includes the network namespace, so the run has no network.
   - Results come back as a tar stream on stdout. The workspace is deleted after each run.
   - Starting limits, to be confirmed on fixtures: build 120 s, test run 30 s, 1 GB memory, 64 processes, 50 MB file size.
5. **Smoke tests first:** run the existing trusted fixtures (`tests/fixtures/functional`) through the WSL executor. Expect the same passed/failed/error/timeout outcomes as the host fixture tests. Also add negative tests: a network connect must fail, writing outside `/ws` must fail, and a fork bomb must hit the process limit. Only then run generated artifacts.

## Risks and how they are handled

| risk | handling |
|---|---|
| WSL2 shares the host's network through NAT | bubblewrap `--unshare-all` gives each test an empty network namespace. The negative test checks it. |
| Linux code reaching Windows files or executables | Automount and interop are disabled. Files move only through tar streams. |
| Distros share one WSL VM kernel | Use a dedicated distro with no other data, no host mounts and no secrets. Kernel escape would need a WSL/Linux kernel vulnerability, which is accepted as low risk for this workload. |
| bubblewrap needs unprivileged user namespaces | Must be checked on the WSL kernel (`bwrap --unshare-all true`). If blocked, use a setuid bwrap or rootless Podman, and report it before proceeding. |
| Resource exhaustion (fork bombs, memory, disk) | prlimit, timeout with kill, tmpfs and file-size limits, and the WSL VM memory cap (`.wslconfig` `memory=6GB`). |
| Disk space | VHDX on D: (36.5 GB free). C: and F: are not used. |
| Reproducibility | Record distro, package versions and the jar hash in the manifest. Optionally export the configured distro (`wsl --export`) as a frozen image. |
| Admin rights or reboot | WSL is already installed, so adding a distro normally needs neither. If Windows asks for elevation, stop and ask. |
| Tests that need services (PostgreSQL, LDAP, HTTP servers) or Python packages (cryptography, ldap3, psycopg2) | Not provided by default. Such combinations end as `error` with reason `import/dependency`, or are marked `not_testable` in review. Adding packages is a separate, recorded decision. |

## What I need from you

Approval of steps 1–3 (install distro on D:, harden, install toolchain). Decide separately whether to add the five dev-header packages and any Python packages.
