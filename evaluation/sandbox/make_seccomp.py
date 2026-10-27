#!/usr/bin/env python3
"""Write a classic-BPF seccomp filter for bubblewrap (--seccomp FD) that:
  - kills the process on a foreign architecture or an x32 syscall number,
  - makes socket(AF_VSOCK, ...) fail with EAFNOSUPPORT (vsock is not covered
    by network namespaces and is how WSL talks to the Windows host),
  - allows everything else (other isolation comes from namespaces and rlimits).
Usage: python3 make_seccomp.py OUTPUT_FILE   (x86_64 only)
"""
import struct
import sys

BPF_LD_W_ABS = 0x20     # BPF_LD | BPF_W | BPF_ABS
BPF_JEQ_K = 0x15        # BPF_JMP | BPF_JEQ | BPF_K
BPF_JGE_K = 0x35        # BPF_JMP | BPF_JGE | BPF_K
BPF_RET_K = 0x06        # BPF_RET | BPF_K
AUDIT_ARCH_X86_64 = 0xC000003E
X32_SYSCALL_BIT = 0x40000000
NR_SOCKET = 41
AF_VSOCK = 40
EAFNOSUPPORT = 97
RET_KILL_PROCESS = 0x80000000
RET_ERRNO = 0x00050000
RET_ALLOW = 0x7FFF0000
OFF_NR, OFF_ARCH, OFF_ARG0 = 0, 4, 16   # struct seccomp_data (little endian, low word of args[0])

program = [
    (BPF_LD_W_ABS, 0, 0, OFF_ARCH),
    (BPF_JEQ_K, 1, 0, AUDIT_ARCH_X86_64),     # 1: arch ok -> 3
    (BPF_RET_K, 0, 0, RET_KILL_PROCESS),      # 2
    (BPF_LD_W_ABS, 0, 0, OFF_NR),             # 3
    (BPF_JGE_K, 0, 1, X32_SYSCALL_BIT),       # 4: x32 -> 5, else 6
    (BPF_RET_K, 0, 0, RET_KILL_PROCESS),      # 5
    (BPF_JEQ_K, 0, 3, NR_SOCKET),             # 6: socket -> 7, else 10
    (BPF_LD_W_ABS, 0, 0, OFF_ARG0),           # 7
    (BPF_JEQ_K, 0, 1, AF_VSOCK),              # 8: AF_VSOCK -> 9, else 10
    (BPF_RET_K, 0, 0, RET_ERRNO | EAFNOSUPPORT),  # 9
    (BPF_RET_K, 0, 0, RET_ALLOW),             # 10
]

with open(sys.argv[1], "wb") as fh:
    for code, jt, jf, k in program:
        fh.write(struct.pack("<HBBI", code, jt, jf, k))
