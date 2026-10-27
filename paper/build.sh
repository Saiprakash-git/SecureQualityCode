#!/bin/bash
# Compile the paper with TeX Live (Ubuntu packages) inside the eval-sandbox WSL distro.
# Usage: bash paper/build.sh  -> paper/build/main.pdf and main.log
set -e
export MSYS_NO_PATHCONV=1  # keep Linux paths for wsl.exe under Git Bash
cd "$(dirname "$0")"
mkdir -p build
rm -f build/main.pdf build/main.log  # never leave a stale PDF behind a failed build
wsl.exe -d eval-sandbox -- sh -c 'mkdir -p "$HOME/bin" && cat > "$HOME/bin/compile_tex.sh" && chmod 755 "$HOME/bin/compile_tex.sh"' < compile_tex.sh
tar -cf - main.tex generated figures | wsl.exe -d eval-sandbox -- /home/evalrunner/bin/compile_tex.sh | tar -x -C build
if [ ! -f build/main.pdf ] || grep -q "^!" build/main.log; then echo "LaTeX build FAILED (see paper/build/main.log)" >&2; exit 1; fi
echo "built paper/build/main.pdf"
