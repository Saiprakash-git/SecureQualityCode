#!/bin/bash
# Run one command in a throw-away bubblewrap sandbox inside eval-sandbox.
#
#   run_isolated.sh SRC_DIR TIMEOUT_S MEM_MB NPROC FSIZE_MB OUTFILE -- CMD [ARGS...]
#
# SRC_DIR is bound read-only at /src. /ws (256 MB) and /tmp (64 MB) are fresh,
# size-capped tmpfs mounts, and the root is remounted read-only, so the only
# writable storage is in RAM and bounded; nothing written inside reaches the
# disk. The rest is a read-only view of /usr and /opt/junit, minimal /dev and
# /proc. All namespaces are unshared (no network, own PID/IPC/UTS/user/cgroup),
# capabilities dropped, socket(AF_VSOCK) blocked by seccomp, environment
# cleared, stdin closed. Limits: wall clock (timeout, then SIGKILL), CPU
# seconds, address space, file size, no core dumps. The process limit is set
# inside the sandbox: RLIMIT_NPROC set before bwrap makes the kernel refuse
# the new user namespace (EAGAIN).
# Stdout and stderr go to OUTFILE (outside the sandbox, capped by FSIZE_MB).
# Exit status is the command's, 124 on wall-clock timeout (137 if killed),
# 125 for a setup problem.
set -u
SRC=$1 T=$2 MEM=$3 NPROC=$4 FSIZE=$5 OUT=$6
shift 6
[ "${1:-}" = "--" ] && shift
SECCOMP=/opt/eval-sandbox/seccomp.bpf
WS_BYTES=$((256 * 1024 * 1024)) TMP_BYTES=$((64 * 1024 * 1024))
[ -d "$SRC" ] && [ -f "$SECCOMP" ] || { echo "run_isolated: missing source dir or seccomp filter" >&2; exit 125; }

exec 9<"$SECCOMP"
timeout -k 5 "$T" \
  prlimit --as=$((MEM * 1024 * 1024)) --fsize=$((FSIZE * 1024 * 1024)) --cpu="$T" --core=0 -- \
  bwrap --unshare-all --die-with-parent --new-session --cap-drop ALL --seccomp 9 \
        --clearenv --setenv PATH /usr/bin:/bin --setenv HOME /ws --setenv LANG C.UTF-8 \
        --setenv TMPDIR /tmp --setenv PYTHONDONTWRITEBYTECODE 1 \
        --hostname sandbox \
        --ro-bind /usr /usr --tmpfs /usr/lib/wsl \
        --symlink usr/lib /lib --symlink usr/lib64 /lib64 --symlink usr/bin /bin --symlink usr/sbin /sbin \
        --ro-bind /etc/alternatives /etc/alternatives \
        --ro-bind /etc/ld.so.cache /etc/ld.so.cache \
        --ro-bind-try /etc/java-21-openjdk /etc/java-21-openjdk \
        --ro-bind /opt/junit /opt/junit \
        --proc /proc --dev /dev \
        --size "$TMP_BYTES" --tmpfs /tmp \
        --size "$WS_BYTES" --tmpfs /ws \
        --ro-bind "$SRC" /src \
        --remount-ro / \
        --chdir /ws \
        -- prlimit --nproc="$NPROC" -- "$@" </dev/null >"$OUT" 2>&1
