#!/bin/bash
# Distro-level hardening checks for eval-sandbox. Run as the default user:
#   wsl.exe -d eval-sandbox -- bash -s < evaluation/sandbox/check_hardening.sh
# Prints one "CHECK <name> PASS|FAIL <detail>" line per check.
check() { if [ "$2" = "0" ]; then echo "CHECK $1 PASS $3"; else echo "CHECK $1 FAIL $3"; fi; }

[ "$(id -u)" != "0" ] && [ "$(id -un)" = "evalrunner" ]; check default_user_not_root $? "$(id -un)"
! sudo -n true 2>/dev/null; check no_sudo $? ""
n=$(grep -cE ' /mnt/[a-zA-Z](/| )' /proc/mounts); [ "$n" = "0" ]; check no_windows_drive_mounts $? "count=$n"
n=$(grep -E ' (drvfs|9p|virtiofs) ' /proc/mounts | grep -cv ' /usr/lib/wsl/drivers '); [ "$n" = "0" ]; check no_other_windows_fs_mounts $? "count=$n"
# /usr/lib/wsl/drivers (read-only 9p view of the Windows GPU driver store) is created by WSL
# itself; the per-test bubblewrap sandbox hides it (checked in check_bwrap.sh).
echo "INFO known_mount $(grep ' /usr/lib/wsl/drivers ' /proc/mounts | cut -d' ' -f1-4)"
[ ! -e /proc/sys/fs/binfmt_misc/WSLInterop ] && [ ! -e /proc/sys/fs/binfmt_misc/WSLInterop-late ]; check interop_binfmt_absent $? "$(ls /proc/sys/fs/binfmt_misc 2>/dev/null | tr '\n' ' ')"
! echo "$PATH" | grep -qi '/mnt/'; check no_windows_path $? "$PATH"
[ -z "$(command -v cmd.exe powershell.exe wsl.exe 2>/dev/null)" ]; check no_windows_exes_on_path $? ""
[ "$(cat /proc/1/comm)" != "systemd" ]; check systemd_off $? "pid1=$(cat /proc/1/comm)"
[ "$(stat -c %U /opt/junit)" = "root" ] && [ ! -w /opt/junit ]; check toolchain_not_writable $? "$(stat -c '%U %a' /opt/junit)"
echo "INFO kernel $(uname -r)"
echo "INFO os $(. /etc/os-release; echo "$PRETTY_NAME")"
echo "INFO max_user_namespaces $(cat /proc/sys/user/max_user_namespaces)"
echo "INFO apparmor_restrict_unprivileged_userns $(cat /proc/sys/kernel/apparmor_restrict_unprivileged_userns 2>/dev/null || echo n/a)"
