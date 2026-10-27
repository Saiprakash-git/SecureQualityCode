#!/bin/bash
# Receive a workspace tar on stdin, run its .eval_driver.sh in a fresh sandbox,
# print the captured output, and delete everything.
#   receive_and_run.sh TOTAL_S MEM_MB NPROC FSIZE_MB < workspace.tar
# Only numeric arguments are accepted, so nothing from the caller is
# interpreted by a shell here.
set -u
for v in "$@"; do [[ "$v" =~ ^[0-9]+$ ]] || { echo "receive_and_run: bad argument" >&2; exit 125; }; done
[ $# = 4 ] || { echo "receive_and_run: need 4 arguments" >&2; exit 125; }
mkdir -p "$HOME/runs"
d=$(mktemp -d "$HOME/runs/r.XXXXXX")
trap 'rm -rf "$d" "$d.out"' EXIT
tar -x -C "$d" --no-same-owner --no-same-permissions --no-overwrite-dir || exit 125
[ -f "$d/.eval_driver.sh" ] || { echo "receive_and_run: no driver" >&2; exit 125; }
/opt/eval-sandbox/run_isolated.sh "$d" "$1" "$2" "$3" "$4" "$d.out" -- bash /src/.eval_driver.sh
rc=$?
cat "$d.out"
exit $rc
