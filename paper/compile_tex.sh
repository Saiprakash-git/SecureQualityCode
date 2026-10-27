#!/bin/bash
# Runs inside eval-sandbox: read a tar of the paper sources on stdin, run pdflatex
# twice (no shell escape), write a tar with main.pdf and main.log to stdout.
set -u
d=$(mktemp -d); trap 'rm -rf "$d"' EXIT
tar -x -C "$d" || exit 125
cd "$d"
for i in 1 2; do pdflatex -interaction=nonstopmode -halt-on-error -no-shell-escape main.tex >/dev/null 2>&1; done
if [ -f main.pdf ]; then tar -c main.pdf main.log; else tar -c main.log; fi
