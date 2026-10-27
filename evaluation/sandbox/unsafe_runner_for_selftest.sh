#!/bin/bash
# NEGATIVE CONTROL ONLY: same interface as run_isolated.sh but with no isolation.
# Used to prove that check_bwrap.sh detects an unsafe runner. Never use for tests.
SRC=$1 T=$2 MEM=$3 NPROC=$4 FSIZE=$5 OUT=$6; shift 6; [ "${1:-}" = "--" ] && shift
mkdir -p /tmp/unsafe_ws && cd /tmp/unsafe_ws && ln -sfn "$SRC" /tmp/unsafe_src
timeout "$T" "${@//\/src\//$SRC/}" </dev/null >"$OUT" 2>&1
