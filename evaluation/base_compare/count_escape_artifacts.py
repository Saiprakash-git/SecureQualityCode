#!/usr/bin/env python3
"""Count published base-paper C/C++ files (40 benchmark tasks) that contain a
doubled backslash escape such as '\\\\0', "\\\\n" or \\\\" inside source text -
the pattern of the base pipeline's extraction artifact (a literal backslash
followed by an escape letter, which usually breaks compilation). Read-only."""
import collections
import csv
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DOUBLE = re.compile(r'\\\\[0nt"\']')  # two backslash characters then 0 n t " or '
rows = list(csv.DictReader((ROOT / "evaluation/base_compare/base_version_classes.csv").open(encoding="utf-8")))
hits = collections.Counter()
files = []
for r in rows:
    if r["language"] in ("c", "cpp"):
        text = (ROOT / r["published"]).read_text(encoding="utf-8", errors="replace")
        if DOUBLE.search(text):
            hits[(r["language"], r["class"])] += 1
            files.append(r["published"])
print("published C/C++ files with a doubled-backslash escape:", len(files))
for k, v in sorted(hits.items()):
    print(" ", k, v)
