#!/bin/bash
# Security checks for the per-test sandbox (run_isolated.sh). Run as evalrunner:
#   wsl.exe -d eval-sandbox -- bash -s < evaluation/sandbox/check_bwrap.sh
# Each check runs a hostile probe inside the sandbox and passes only if
#   (a) the probe demonstrably ran inside the sandbox (it prints RAN first), and
#   (b) the attack failed in the expected way.
# A sandbox that fails to start therefore fails every check.
# Prints "CHECK <name> PASS|FAIL <detail>".
set -u
RUN=${RUN:-/opt/eval-sandbox/run_isolated.sh}  # override only for the negative-control self-test
BASE=$(mktemp -d /home/evalrunner/bwcheck.XXXX)
trap 'rm -rf "$BASE"; pkill -u evalrunner -f "sleep 4242" 2>/dev/null; pkill -u evalrunner -f "sleep 4343" 2>/dev/null' EXIT
n=0
ws() { n=$((n + 1)); mkdir -p "$BASE/src$n"; echo "$BASE/src$n"; }
check() { if [ "$2" = "0" ]; then echo "CHECK $1 PASS $3"; else echo "CHECK $1 FAIL $3"; fi; }
py() {  # py SRC TIMEOUT MEM_MB NPROC FSIZE_MB LINE...  (output in SRC.out)
  local s=$1 t=$2 mem=$3 np=$4 fs=$5; shift 5
  { echo 'print("RAN", flush=True)'; printf '%s\n' "$@"; } > "$s/p.py"
  $RUN "$s" "$t" "$mem" "$np" "$fs" "$s.out" -- python3 /src/p.py
}
ran() { grep -q '^RAN$' "$1.out"; }
show() { grep -v '^RAN$' "$1.out" | tr '\n' ' ' | head -c 300; }

echo secret > /home/evalrunner/canary.txt
export SECRET_CANARY=leak-me
sleep 4242 & CANARY_PID=$!

# 0. Control: a normal program runs, can write to /ws, and nothing reaches the host disk
S=$(ws); py "$S" 10 512 32 10 'import os; open("/ws/ok.txt","w").write("x"); print("hello", os.listdir("/ws"))'; rc=$?
ran "$S" && [ $rc = 0 ] && grep -q "hello \['ok.txt'\]" "$S.out" && [ ! -e "$S/ok.txt" ]; check control_runs $? "rc=$rc $(show "$S")"

# 1. Identity, capabilities, no_new_privs
S=$(ws); py "$S" 10 512 32 10 'import os,re' 's=open("/proc/self/status").read()' \
  'print("ID", os.getuid(), re.search(r"CapEff:\s*(\w+)",s).group(1), re.search(r"NoNewPrivs:\s*(\d)",s).group(1))'
read -r _ uid cap nnp < <(grep '^ID ' "$S.out")
ran "$S" && [ "$uid" != "0" ] && [ "$cap" = "0000000000000000" ] && [ "$nnp" = "1" ]; check no_root_no_caps_nonewprivs $? "uid=$uid cap=$cap nnp=$nnp"

# 2. Network: only loopback, no TCP, no DNS
S=$(ws); py "$S" 20 512 32 10 'import socket' 'print("IF", sorted(n for _, n in socket.if_nameindex()))' \
  'for host,port in (("1.1.1.1",53),("8.8.8.8",443),("172.17.0.1",80)):' \
  '    try: socket.create_connection((host,port),3); print("CONNECTED",host)' \
  '    except OSError as e: print("blocked",host,e.errno)' \
  'try: socket.getaddrinfo("example.com",80); print("DNS_OK")' \
  'except OSError: print("dns blocked")'
ran "$S" && grep -q "IF \['lo'\]" "$S.out" && [ "$(grep -c '^blocked' "$S.out")" = 3 ] && grep -q "dns blocked" "$S.out" \
  && ! grep -qE "CONNECTED|DNS_OK" "$S.out"; check network_blocked $? "$(show "$S")"

# 3. vsock (the WSL host channel) is refused by seccomp
S=$(ws); py "$S" 10 512 32 10 'import socket' 'try: socket.socket(40, socket.SOCK_STREAM); print("VSOCK_OPEN")' 'except OSError as e: print("vsock blocked", e.errno)'
ran "$S" && grep -q "vsock blocked 97" "$S.out"; check vsock_blocked $? "$(show "$S")"

# 4. Filesystem: read-only root/system/src, no home, no Windows files, no secrets, minimal /dev
S=$(ws); py "$S" 10 512 32 10 'import os' \
  'def can_write(p):' '    try: open(p,"w").write("x"); return True' '    except OSError: return False' \
  'for p in ("/usr/bin/evil","/evil","/opt/junit/x","/src/x","/etc/x","/proc/x"): print("w", p, can_write(p))' \
  'print("home", os.path.exists("/home/evalrunner/canary.txt")); print("mnt", os.path.exists("/mnt")); print("shadow", os.path.exists("/etc/shadow"))' \
  'print("wsl_drivers", os.listdir("/usr/lib/wsl")); print("init", os.path.exists("/init"))' \
  'print("dev", sorted(os.listdir("/dev")))'
ran "$S" && [ "$(grep -c '^w .* False$' "$S.out")" = 6 ] \
  && grep -q "home False" "$S.out" && grep -q "mnt False" "$S.out" && grep -q "shadow False" "$S.out" \
  && grep -q "wsl_drivers \[\]" "$S.out" && grep -q "init False" "$S.out" \
  && ! grep -qE "'(sd[a-z]|vd[a-z]|dxg|kmsg|mem|vsock)'" "$S.out"
check filesystem_confined $? "$(show "$S")"

# 5. Writable space is capped: /ws 256 MB, /tmp 64 MB (5 MB files, file-size limit 8 MB)
S=$(ws); py "$S" 30 512 32 8 'import os,errno' \
  'def fill(d, limit_mb):' \
  '    total=0' \
  '    try:' \
  '        for i in range(limit_mb):' \
  '            with open(f"{d}/f{i}","wb") as f: f.write(b"x"*5*1024*1024)' \
  '            total+=5' \
  '    except OSError as e: return total, errno.errorcode.get(e.errno)' \
  '    return total, None' \
  'print("ws", *fill("/ws", 120)); print("tmp", *fill("/tmp", 40))'
read -r _ ws_mb ws_err < <(grep '^ws ' "$S.out"); read -r _ tmp_mb tmp_err < <(grep '^tmp ' "$S.out")
ran "$S" && [ "$ws_err" = ENOSPC ] && [ "${ws_mb:-999}" -le 256 ] && [ "$tmp_err" = ENOSPC ] && [ "${tmp_mb:-999}" -le 64 ]
check writable_space_capped $? "ws=${ws_mb}MB:$ws_err tmp=${tmp_mb}MB:$tmp_err"

# 6. Environment not inherited
S=$(ws); py "$S" 10 512 32 10 'import os; print("ENV", sorted(os.environ))'
ran "$S" && grep -q '^ENV ' "$S.out" && ! grep -q SECRET_CANARY "$S.out"; check env_cleared $? "$(show "$S")"

# 7. PID isolation: cannot see or signal processes outside
S=$(ws); py "$S" 10 512 32 10 'import os,signal' "pid=$CANARY_PID" \
  'print("procs", len([p for p in os.listdir("/proc") if p.isdigit()]))' \
  'try: os.kill(pid, signal.SIGKILL); print("KILLED_OUTSIDE")' 'except OSError as e: print("kill blocked", e.errno)'
procs=$(sed -n 's/^procs //p' "$S.out")
ran "$S" && kill -0 "$CANARY_PID" 2>/dev/null && grep -q "kill blocked" "$S.out" && [ "${procs:-99}" -le 4 ]
check pid_isolated $? "$(show "$S")"

# 8. Memory limit (address space 256 MB; try 1 GB)
S=$(ws); py "$S" 20 256 32 10 'try:' '    b = bytearray(1024*1024*1024); print("ALLOCATED_1GB")' 'except MemoryError: print("memory limited")'
ran "$S" && grep -q "memory limited" "$S.out"; check memory_limited $? "$(show "$S")"

# 9. Process limit (fork bomb, limit 16)
S=$(ws); py "$S" 20 512 16 10 'import os,time' 'kids=0' 'try:' \
  '    for _ in range(200):' '        if os.fork()==0: time.sleep(5); os._exit(0)' '        kids+=1' \
  'except OSError as e: print("fork limited after", kids, e.errno)' 'else: print("FORKED_ALL", kids)'
ran "$S" && grep -q "fork limited" "$S.out"; check process_limited $? "$(show "$S")"

# 10. File size limit (5 MB; try 20 MB in one file)
S=$(ws); py "$S" 20 512 32 5 'import os,signal; signal.signal(signal.SIGXFSZ, signal.SIG_IGN)' 'try:' \
  '    open("/ws/big","wb").write(b"x"*(20*1024*1024)); print("WROTE_20MB")' 'except OSError as e: print("fsize limited", e.errno, os.path.getsize("/ws/big"))'
sz=$(sed -n 's/^fsize limited [0-9]* //p' "$S.out")
ran "$S" && grep -q "^fsize limited 27 " "$S.out" && [ "${sz:-0}" -le $((5*1024*1024)) ]; check filesize_limited $? "$(show "$S")"  # 27 = EFBIG

# 11. Output file outside the sandbox is capped by the file-size limit
S=$(ws); py "$S" 20 512 32 2 'import sys,signal; signal.signal(signal.SIGXFSZ, signal.SIG_IGN)' 'try:' \
  '    sys.stdout.write("y"*(10*1024*1024)); sys.stdout.flush()' 'except OSError: pass'
osz=$(stat -c %s "$S.out"); ran "$S" && [ "$osz" -le $((2*1024*1024)) ]; check output_capped $? "size=$osz"

# 12. /tmp is private
touch /tmp/host_tmp_canary
S=$(ws); py "$S" 10 512 32 10 'import os; print("host_tmp", os.path.exists("/tmp/host_tmp_canary"))'
ran "$S" && grep -q "host_tmp False" "$S.out"; check tmp_private $? "$(show "$S")"; rm -f /tmp/host_tmp_canary

# 13. Wall-clock timeout kills a sleeping program; CPU limit kills a busy one
S=$(ws); start=$(date +%s); py "$S" 3 512 32 10 'import time; time.sleep(60)'; rc=$?; el=$(( $(date +%s) - start ))
ran "$S" && { [ $rc = 124 ] || [ $rc = 137 ]; } && [ $el -le 10 ]; check wall_timeout $? "rc=$rc elapsed=${el}s"
S=$(ws); start=$(date +%s); py "$S" 3 512 32 10 'while True: pass'; rc=$?; el=$(( $(date +%s) - start ))
ran "$S" && [ $rc != 0 ] && [ $el -le 10 ]; check cpu_timeout $? "rc=$rc elapsed=${el}s"

# 14. No processes survive the sandbox (daemonised grandchild)
S=$(ws); py "$S" 10 512 32 10 'import subprocess' 'subprocess.Popen(["sleep","4343"], start_new_session=True)' 'print("spawned")'
sleep 1; left=$(pgrep -u evalrunner -f "sleep 4343" | wc -l)
ran "$S" && grep -q spawned "$S.out" && [ "$left" = 0 ]; check no_survivors $? "left=$left"

# 15. Windows executables cannot run (interop binfmt removed, /init absent).
# Without a binfmt handler the kernel returns ENOEXEC; execvp may then hand the
# file to /bin/sh, which fails on the binary. Either way it must not run as a
# Windows program (whoami would print HOST\user).
S=$(ws); cp /opt/eval-sandbox/probe.exe "$S/probe.exe" && chmod +x "$S/probe.exe"
$RUN "$S" 10 512 32 10 "$S.ctl" -- /bin/true; ctl=$?
$RUN "$S" 10 512 32 10 "$S.out" -- /src/probe.exe; rc=$?
[ $ctl = 0 ] && [ $rc != 0 ] && grep -qaiE 'exec format error|cannot execute|not found' "$S.out" \
  && ! grep -qaE '^[A-Za-z0-9-]+\\[A-Za-z0-9._-]+$' "$S.out"
check windows_exe_blocked $? "control=$ctl rc=$rc"

# 16. Toolchains work inside the sandbox (controls for gate 2; build and run in one sandbox)
S=$(ws); printf '#include <stdio.h>\n#include <pthread.h>\nint main(void){puts("c ok");return 0;}\n' > "$S/t.c"
$RUN "$S" 60 1024 64 50 "$S.out" -- sh -c 'gcc -O0 -o /ws/t /src/t.c -lm -lpthread && /ws/t'; rc=$?
[ $rc = 0 ] && grep -q "c ok" "$S.out"; check gcc_works $? "rc=$rc $(head -c 200 "$S.out")"
S=$(ws); printf '#include <iostream>\n#include <thread>\nint main(){std::thread t([]{std::cout<<"cpp ok\\n";});t.join();}\n' > "$S/t.cpp"
$RUN "$S" 90 1024 64 50 "$S.out" -- sh -c 'g++ -std=gnu++17 -O0 -o /ws/t /src/t.cpp -lpthread && /ws/t'; rc=$?
[ $rc = 0 ] && grep -q "cpp ok" "$S.out"; check gxx_works $? "rc=$rc $(head -c 200 "$S.out")"
S=$(ws); printf 'public class T { public static void main(String[] a){ System.out.println("java ok"); } }\n' > "$S/T.java"
$RUN "$S" 90 4096 128 50 "$S.out" -- sh -c 'javac -d /ws /src/T.java && java -Xmx256m -cp /ws T'; rc=$?
[ $rc = 0 ] && grep -q "java ok" "$S.out"; check java_works $? "rc=$rc $(head -c 200 "$S.out")"
S=$(ws); $RUN "$S" 60 4096 128 50 "$S.out" -- java -Xmx256m -jar /opt/junit/junit-platform-console-standalone-1.11.4.jar --version; rc=$?
[ $rc = 0 ] && grep -q "1.11.4" "$S.out"; check junit_works $? "rc=$rc"
S=$(ws); py "$S" 20 512 32 10 'import unittest, sys; print(sys.version.split()[0])'
ran "$S"; check python_works $? "$(show "$S")"

kill "$CANARY_PID" 2>/dev/null
rm -f /home/evalrunner/canary.txt
echo "INFO bwrap $(bwrap --version | cut -d' ' -f2) gcc $(gcc -dumpfullversion) python $(python3 -V 2>&1 | cut -d' ' -f2) java $(java -version 2>&1 | head -1 | cut -d'"' -f2) junit 1.11.4"
