#!/usr/bin/env python3
"""Consistency checks for paper/main.tex: citations vs bibliography, refs vs
labels, and every literal number in the body (to confirm it is either a fixed
fact or comes from generated/numbers.tex)."""
import re
from pathlib import Path

t = (Path(__file__).resolve().parents[1] / "main.tex").read_text(encoding="utf-8")
a, b = t.find(r"\begin{abstract}"), t.find(r"\begin{thebibliography}")
body = re.sub(r"(?<!\\)%.*", "", t[a:b])
cites = {k.strip() for g in re.findall(r"\\cite\{([^}]*)\}", t) for k in g.split(",")}
bib = set(re.findall(r"\\bibitem\{([^}]*)\}", t))
labels = set(re.findall(r"\\label\{([^}]*)\}", t))
refs = set(re.findall(r"\\ref\{([^}]*)\}", t))
print("cited but missing from bibliography:", sorted(cites - bib))
print("bibliography entries never cited:  ", sorted(bib - cites))
print("refs without label:", sorted(refs - labels), "| labels never referenced:", sorted(labels - refs))
txt = re.sub(r"\\(cite|ref|label|input|includegraphics|texttt|url)\{[^}]*\}", "", body)
for m in re.finditer(r"(?<![\\\w])\d[\d.,]*", txt):
    ctx = txt[max(0, m.start() - 45):m.end() + 20].replace("\n", " ")
    print(f"{m.group():>8} | {ctx}")
