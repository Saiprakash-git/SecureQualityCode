#!/bin/bash
# Static security/quality scanners over a snapshot (nothing is executed).
#   run_scanners.sh < input.tar > output.tar
# input.tar: src/{python,java,c,cpp}/<combo>.<ext>, rules/*.yml
# output.tar: out/semgrep.json, bandit.json, flawfinder.csv, cppcheck_c.xml,
#             cppcheck_cpp.xml, lizard.csv, versions.txt
set -u
S=/opt/scanners/bin
d=$(mktemp -d "$HOME/scan.XXXXXX"); trap 'rm -rf "$d"' EXIT
tar -x -C "$d" --no-same-owner --no-same-permissions || exit 125
cd "$d"; mkdir -p out
export SEMGREP_SEND_METRICS=off SEMGREP_ENABLE_VERSION_CHECK=0 HOME="$d"
{
  echo "semgrep $($S/semgrep --version 2>/dev/null)"
  echo "bandit $($S/bandit --version 2>/dev/null | head -1)"
  echo "flawfinder $($S/flawfinder --version 2>/dev/null)"
  echo "lizard $($S/lizard --version 2>/dev/null)"
  echo "cppcheck $(cppcheck --version)"
  sha256sum rules/*.yml
} > out/versions.txt
$S/semgrep scan --config rules/cwe-top-25.yml --config rules/security-audit.yml \
  --metrics=off --disable-version-check --no-git-ignore --timeout 30 --jobs 4 --max-target-bytes 2000000 \
  --json --output out/semgrep.json src >out/semgrep.log 2>&1
$S/bandit -r src/python -f json -o out/bandit.json -q >out/bandit.log 2>&1
$S/flawfinder --csv --minlevel=0 src/c src/cpp > out/flawfinder.csv 2>out/flawfinder.log
cppcheck --enable=warning,style,performance,portability --inline-suppr --language=c --std=c17 \
  --xml --xml-version=2 -j 4 src/c 2> out/cppcheck_c.xml >/dev/null
cppcheck --enable=warning,style,performance,portability --inline-suppr --language=c++ --std=c++17 \
  --xml --xml-version=2 -j 4 src/cpp 2> out/cppcheck_cpp.xml >/dev/null
$S/lizard -l python -l java -l cpp --csv src > out/lizard.csv 2>out/lizard.log
tar -c -C "$d" out
