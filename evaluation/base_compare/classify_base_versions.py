#!/usr/bin/env python3
"""Classify, for the 40 benchmark tasks, how each base-paper output in
dataset/output (published) differs from the copy in the base paper's test
repositories (evaluation/.upstream, pinned commits), which is what the base
paper's CodeQL/SonarCloud reports analysed. Read-only; writes one CSV.

Classes (first match wins):
  identical / whitespace_only
  harness_main_removed      test copy has no main() / __main__ block that the published copy has
  harness_commented_out     differences are only lines commented out in the test copy
  escape_artifact_fixed     published copy has \\" or \\n inside string literals that the test copy fixes
  other_edit                any other change (unclassified; needs manual inspection)
Java has no published copy (dataset/output contains no .java files)."""
import collections
import csv
import difflib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
D = ROOT / "Repo/SecureQualityCode/code-llm-evaluation-dataset-main/code-llm-evaluation-dataset-main/dataset/output"
OURS = {0, 3, 9, 12, 14, 15, 17, 18, 21, 23, 25, 28, 31, 44, 56, 66, 67, 68, 72, 73, 74, 76, 78, 82, 84, 86, 89, 91,
        99, 101, 105, 113, 114, 124, 130, 132, 143, 145, 148, 149}
DIRS = {"CLAUDE_claude-3-5-sonnet-20240620": ("claude",), "GEMINI_gemini-1.5-pro-001": ("gemini",),
        "MISTRAL_codestral-latest": ("codestral",), "OPENAI_gpt-4o": ("gtp4o", "gpt4o"),
        "PERPLEXITY_llama-3-sonar-large-32k-chat": ("llama3", "llama-3")}
MAIN = {"python": re.compile(r"^\s*if\s+__name__\s*==", re.M), "c": re.compile(r"\bint\s+main\s*\("),
        "cpp": re.compile(r"\bint\s+main\s*\(")}


def classify(lang, pub, test):
    if pub == test:
        return "identical"
    if re.sub(r"\s+", "", pub) == re.sub(r"\s+", "", test):
        return "whitespace_only"
    if MAIN[lang].search(pub) and not MAIN[lang].search(test):
        return "harness_main_removed"
    a, b = pub.splitlines(), test.splitlines()
    changed = [l for l in difflib.ndiff(a, b) if l[:2] in ("- ", "+ ")]
    added = [l[2:] for l in changed if l.startswith("+ ")]
    removed = [l[2:].strip() for l in changed if l.startswith("- ")]
    cmt = "#" if lang == "python" else "//"
    if added and all(x.strip().startswith(cmt) or not x.strip() for x in added) and \
            all(any(x.strip().lstrip(cmt).strip() == r or not r for x in added) for r in removed):
        return "harness_commented_out"
    if re.search(r'\\\\[n"t]', pub) and not re.search(r'\\\\[n"t]', test):
        return "escape_artifact_fixed"
    return "other_edit"


def main():
    man = json.loads((ROOT / "evaluation/.upstream/manifest.json").read_text(encoding="utf-8"))
    up = collections.defaultdict(dict)
    for f in man["files"]:
        if f["role"] == "implementation":
            up[(f["language"], f["task_id"])][f["original_model"]] = ROOT / "evaluation" / f["local_path"]
    rows = []
    for d, aliases in DIRS.items():
        for f in sorted((D / d).iterdir()):
            m = re.match(r"Task(\d+)_.*\.(py|c|cpp)$", f.name)
            if not m or int(m.group(1)) not in OURS:
                continue
            lang = {"py": "python", "c": "c", "cpp": "cpp"}[m.group(2)]
            t = int(m.group(1))
            test = next((up[(lang, t)][a] for a in aliases if a in up[(lang, t)]), None)
            pub = f.read_text(encoding="utf-8", errors="replace").replace("\r\n", "\n")
            cls = "no_test_copy" if test is None else classify(lang, pub, test.read_text(encoding="utf-8", errors="replace").replace("\r\n", "\n"))
            rows.append({"model": d.split("_")[0], "task_id": t, "language": lang, "published": str(f.relative_to(ROOT)).replace("\\", "/"),
                         "test_copy": "" if test is None else str(test.relative_to(ROOT)).replace("\\", "/"), "class": cls})
    out = ROOT / "evaluation/base_compare/base_version_classes.csv"
    with out.open("w", encoding="utf-8", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(rows[0]), lineterminator="\n")
        w.writeheader()
        w.writerows(rows)
    print(len(rows), "files ->", out.relative_to(ROOT))
    for k, v in sorted(collections.Counter((r["language"], r["class"]) for r in rows).items()):
        print(f"  {k[0]:7s} {k[1]:24s} {v}")
    print("by model (non-identical):", dict(collections.Counter(r["model"] for r in rows if r["class"] not in ("identical", "whitespace_only"))))


if __name__ == "__main__":
    main()
