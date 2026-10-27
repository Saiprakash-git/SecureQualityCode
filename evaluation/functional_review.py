#!/usr/bin/env python3
"""
functional_review.py - Build the human-review reports for the proposed
functional-test oracles and adapters. Nothing is approved here.

Writes to evaluation/functional/review/:
  ORACLE_REVIEW.md, oracle_review_template.json
  ADAPTER_REVIEW.md, adapter_review_template.json
Reviewers copy decided entries into functional/oracle_review.json and
functional/adapter_review.json (same keys); only those files are read by
functional.py, and only entries whose hashes still match are honoured.

Usage: python functional_review.py --plan results/<functional plan folder>
"""

import argparse
import csv
import json
import re
from collections import Counter, defaultdict
from pathlib import Path

import functional as F

EVAL_DIR = F.EVAL_DIR
REVIEW_DIR = F.FUNCTIONAL_DIR / "review"
LANG_ORDER = ["java", "python", "c", "cpp"]

# Heuristic warning flags. They point the reviewer at things to check; they
# are not judgments and never approve or reject anything.
FLAG_PATTERNS = {
    "file_io": r"\b(?:open\(|File(?:Reader|Writer|InputStream|OutputStream)?\b|fopen|ofstream|ifstream|Files\.|tempfile|createTempFile)",
    "network": r"\b(?:socket|Socket|URL\(|HttpClient|requests\.|urllib|connect\()",
    "time_or_random": r"\b(?:time\.|sleep|Thread\.sleep|System\.currentTimeMillis|LocalDate|datetime|random|Random|rand\(|srand)",
    "stdin_stdout_capture": r"\b(?:System\.setOut|System\.setIn|sys\.stdout|redirect_stdout|capsys|StringIO|freopen|cin\b|scanf|std::cout\.rdbuf)",
    "expects_exception": r"\b(?:assertThrows|assertRaises|pytest\.raises|EXPECT_THROW|expected\s*=\s*\w+Exception)",
    "float_compare": r"assert(?:Equals|Equal)\s*\([^;\n]*\d\.\d",
    "subprocess_or_threads": r"\b(?:subprocess|ProcessBuilder|Runtime\.getRuntime|fork\(|pthread_create|std::thread|ExecutorService|threading)",
    "database_or_service": r"\b(?:sqlite|jdbc|psycopg|ldap|LDAP|mysql|DriverManager)",
}
CHECK_LINE = {
    "python": re.compile(r"self\.assert\w+\(|^\s*assert\s|pytest\.raises"),
    "java": re.compile(r"\bassert\w*\s*\(|\bfail\s*\("),
    "c": re.compile(r"\bassert\s*\(|\bFAIL\b|\brun_test\s*\(|expected"),
    "cpp": re.compile(r"\bassert\s*\(|\bFAIL\b|ASSERT_|EXPECT_|\brun_test\s*\(|expected"),
}


def load_prompts() -> dict:
    root = EVAL_DIR.parent / "Repo/SecureQualityCode/code-llm-evaluation-dataset-main/code-llm-evaluation-dataset-main/dataset"
    prompts = {}
    for jf in sorted(root.glob("promptDescription-*/promptDescription-*.json")):
        task = json.loads(jf.read_text(encoding="utf-8"))["tasks"][0]
        prompts[task["task_number"]] = task
    return prompts


def literals(text: str) -> list[str]:
    text = re.sub(r"(?m)^\s*(?:#|//).*$", "", text)
    return F._LITERAL.findall(text)


def spec_overlap(test_text: str, prompt: str) -> tuple[int, int, list[str]]:
    """How many distinct non-trivial literals of the test also occur in the prompt text."""
    lits = {l.strip("\"'") for l in literals(test_text)}
    lits = {l for l in lits if len(l) >= 2 and l not in {"10", "0.0", "1.0"}}
    found = sorted(l for l in lits if l and l in prompt)
    return len(found), len(lits), found[:12]


def check_lines(language: str, text: str, limit: int = 40) -> list[str]:
    out = []
    for no, line in enumerate(text.splitlines(), start=1):
        if CHECK_LINE[language].search(line):
            out.append(f"{no:4d}: {line.rstrip()[:160]}")
    return out[:limit] + ([f"      ... {len(out) - limit} more"] if len(out) > limit else [])


def flags_for(entry: dict, ref: dict, test_text: str) -> list[str]:
    flags = []
    n_usable = entry["usable_files"]
    agree = int(entry["agreement"].split("/")[0])
    if n_usable > 1 and agree == 1:
        flags.append(f"agreement_single: no other usable test has the same values ({entry['agreement']})")
    elif n_usable > 1 and agree < n_usable:
        flags.append(f"agreement_partial: {entry['agreement']} usable tests share these values")
    if n_usable == 1:
        flags.append("only_one_usable_test: nothing to cross-check against")
    if ref["checks"] < 3:
        flags.append(f"few_checks: {ref['checks']}")
    if any(s.get("method", s["canonical"]) == "main" or s["canonical"].endswith(".main") for s in ref["symbols"]):
        flags.append("drives_main: test calls main(), so behaviour is checked through program I/O")
    if ref.get("instance_use"):
        flags.append("instance_use: test instantiates the original class (adapter needs a constructor)")
    for name, rx in FLAG_PATTERNS.items():
        if re.search(rx, test_text):
            flags.append(name)
    return flags


def other_tests_summary(entry: dict, ref: dict) -> list[str]:
    ref_lits = Counter(literals(F.read_text(EVAL_DIR / ref["test_local_path"])))
    lines = []
    for t in entry["tests"]:
        if t["test_path"] == ref["test_path"]:
            continue
        why = (t["binding"] if t["binding"] not in F.USABLE_BINDINGS else
               "no checks" if not t["checks"] else "no resolvable symbol" if not t["symbols"] else "?")
        status = "usable" if t["usable"] else f"not usable ({why}, checks={t['checks']})"
        same = "same values" if t.get("case_fingerprint") == ref["case_fingerprint"] else "different values"
        line = f"- {t['original_model']}: {status}; {same if t['usable'] else ''}".rstrip("; ")
        if t["usable"] and t.get("case_fingerprint") != ref["case_fingerprint"]:
            other = Counter(literals(F.read_text(EVAL_DIR / t["test_local_path"])))
            only_ref = sorted((ref_lits - other).elements())[:8]
            only_other = sorted((other - ref_lits).elements())[:8]
            line += f"; only in reference: {only_ref}; only in {t['original_model']}: {only_other}"
        lines.append(line)
    return lines


def oracle_report(catalog: dict, prompts: dict, plan_rows: list[dict]) -> tuple[str, dict]:
    combos_by_key = defaultdict(list)
    for r in plan_rows:
        if r["oracle_status"] != "no_oracle":
            combos_by_key[f"{r['task_id']}/{r['language']}"].append(r)
    entries = [e for e in catalog["entries"].values() if e["reference"]]

    def sort_key(e):
        agree, n = (int(x) for x in e["agreement"].split("/"))
        weak = 0 if (n > 1 and agree == 1) else 1 if n == 1 else 2
        return LANG_ORDER.index(e["language"]), weak, e["task_id"]

    entries.sort(key=sort_key)
    template, md = {}, []
    counts = Counter(e["language"] for e in entries)
    md += ["# Functional-test oracle review", "",
           "Status: **nothing is approved.** Every decision below is open. An oracle is used only after a "
           "reviewer records a decision in `functional/oracle_review.json`, with the `test_sha256` shown here.", "",
           "**How to review one oracle.** Read the task prompt, then the checks. Approve only if every expected "
           "value follows from the prompt, and the test does not depend on details the prompt leaves open: names, "
           "exception types, output format, files, time, randomness. Compiling or matching function names is not "
           "evidence that a test is correct.", "",
           "Flags are automated hints, not judgments. *Spec overlap* counts the test's literal values that also "
           "appear in the prompt text. A high count means the examples were taken from the prompt, but that "
           "still has to be checked.", "",
           f"Entries: {len(entries)} ({', '.join(f'{l} {counts[l]}' for l in LANG_ORDER)}). "
           "Order: Java first (single-test references first), then Python, C, C++.", ""]
    for e in entries:
        key = f"{e['task_id']}/{e['language']}"
        ref = next(t for t in e["tests"] if t["test_path"] == e["reference"]["test_path"])
        text = F.read_text(EVAL_DIR / ref["test_local_path"])
        prompt = prompts[e["task_id"]]
        hit, total, found = spec_overlap(text, prompt["prompt_description"])
        n_combos = len(combos_by_key.get(key, []))
        n_gate1 = sum(1 for r in combos_by_key.get(key, []) if r["gate1_status"] == "pass")
        md += [f"## {key}: {prompt.get('prompt_title') or '(untitled)'}", "",
               f"- Reference: `{ref['test_path']}` ({ref['original_model']}, {ref['framework']}, "
               f"{ref['checks']} checks), agreement {e['agreement']}",
               f"- test_sha256: `{ref['test_sha256']}`",
               "- Symbols: " + "; ".join(
                   f"`{s['canonical']}`" + (f" ({s.get('return', '')} ← {', '.join(s.get('params', []))})" if "params" in s
                                            else f" (arity {s.get('arity')})" if s.get("arity") is not None else "")
                   for s in ref["symbols"]),
               f"- Would test {n_gate1} gate-1-passing artifacts ({n_combos} combinations use this oracle)",
               f"- Spec overlap: {hit}/{total} literals appear in the prompt" + (f": {found}" if found else ""),
               "- Flags: " + ("; ".join(flags_for(e, ref, text)) or "none"), "",
               "<details><summary>Prompt</summary>", "", prompt["prompt_description"].strip(), "", "</details>", "",
               "Checks:", "", "```", *check_lines(e["language"], text), "```", ""]
        others = other_tests_summary(e, ref)
        if others:
            md += ["Other models' tests:", "", *others, ""]
        md += ["Decision: ☐ approve ☐ reject. Notes: ____", ""]
        template[key] = {"approved": None, "test_sha256": ref["test_sha256"], "reviewer": "", "notes": "",
                         "reference_test": ref["test_path"], "agreement": e["agreement"]}
    return "\n".join(md) + "\n", template


def adapter_report(plan_dir: Path, plan_rows: list[dict]) -> tuple[str, dict]:
    adapters = {}
    for f in sorted((plan_dir / "adapters").glob("*.json")):
        a = json.loads(f.read_text(encoding="utf-8"))
        adapters[a["combo_index"]] = a
    rows = [r for r in plan_rows if int(r["combo_index"]) in adapters]
    rows.sort(key=lambda r: (LANG_ORDER.index(r["language"]), int(r["task_id"]), r["model_label"]))
    status_counts = Counter(adapters[int(r["combo_index"])]["status"] for r in rows)
    md = ["# Adapter review", "",
          "Status: **nothing is approved.** An adapter is used only when "
          "`functional/adapter_review.json` holds `approved: true` and a reviewer for the exact artifact and test "
          "hashes. Adapters bind names only; they never add logic.", "",
          "Proposal labels are not approvals:", "",
          "- **proposed_exact**: same name and arity, and the same declared parameter/return types where the "
          "language has them (Python has none to compare).",
          "- **proposed_loose**: a name match that ignores case and `_`; or an exact name with different types; "
          "or a binding to `main`.",
          "- **unresolved**: no unique candidate. The reviewer supplies the binding, or rejects the adapter.", "",
          "Check that the target really implements what the oracle's symbol does, with compatible argument order "
          "and meaning. A same-named function with different semantics must be rejected.", "",
          f"Adapters: {len(rows)} ({', '.join(f'{k} {v}' for k, v in sorted(status_counts.items()))}). Only "
          "artifacts that passed gate 1 have adapters. Review an adapter only after its oracle is approved.", ""]
    template = {}
    current = None
    for r in rows:
        a = adapters[int(r["combo_index"])]
        group = f"{r['task_id']}/{r['language']}"
        if group != current:
            md += [f"## {group}", ""]
            current = group
        md.append(f"### #{r['combo_index']} {r['model_label']}: {a['status']}")
        md.append(f"- artifact: `{a['artifact_path']}` (`{a['artifact_sha256'][:16]}…`)")
        for b in a["bindings"]:
            if b.get("target"):
                cs, ts = b.get("canonical_signature", {}), b.get("target_signature", {})
                fmt = lambda s: (f"{s.get('return') or '?'} ({', '.join(s.get('params') or [])})" if s.get("params") is not None
                                 else f"arity {s.get('arity')}")
                md.append(f"- `{b['canonical']}` → `{b['target']}` [{b.get('name_match')} name, "
                          f"types {'match' if b.get('types_match') else 'n/a' if b.get('types_match') is None else 'DIFFER'}]: "
                          f"oracle `{fmt(cs)}` vs artifact `{fmt(ts)}`")
            else:
                md.append(f"- `{b['canonical']}` → **unresolved**; same/similar names: {b.get('candidates') or 'none'}")
        if a["status"] == "unresolved":
            available = F.artifact_symbols(r["language"], F.read_text(EVAL_DIR.parent / a["artifact_path"]))
            listing = ", ".join(f"`{n}`/{v.get('arity')}" for n, v in sorted(available.items())[:15])
            md.append(f"- artifact symbols (name/arity): {listing or 'none found'}")
        md += ["- Decision: ☐ approve ☐ reject ☐ approve with binding: ____", ""]
        template[str(r["combo_index"])] = {
            "approved": None, "reviewer": "", "notes": "", "status_proposed": a["status"],
            "artifact_sha256": a["artifact_sha256"], "oracle_test_sha256": a["oracle_test_sha256"],
            "bindings": [{"canonical": b["canonical"], "target": b.get("target")} for b in a["bindings"]],
        }
    return "\n".join(md) + "\n", template


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description="Build oracle and adapter review reports (approves nothing).")
    parser.add_argument("--plan", required=True, help="Folder written by `functional.py plan`.")
    args = parser.parse_args(argv)
    plan_dir = Path(args.plan).resolve()
    catalog = json.loads(F.CATALOG_PATH.read_text(encoding="utf-8"))
    with (plan_dir / "functional_plan.csv").open(encoding="utf-8", newline="") as fh:
        plan_rows = list(csv.DictReader(fh))
    REVIEW_DIR.mkdir(parents=True, exist_ok=True)
    md, template = oracle_report(catalog, load_prompts(), plan_rows)
    (REVIEW_DIR / "ORACLE_REVIEW.md").write_text(md, encoding="utf-8", newline="\n")
    (REVIEW_DIR / "oracle_review_template.json").write_text(json.dumps(template, indent=1) + "\n", encoding="utf-8")
    md, a_template = adapter_report(plan_dir, plan_rows)
    (REVIEW_DIR / "ADAPTER_REVIEW.md").write_text(md, encoding="utf-8", newline="\n")
    (REVIEW_DIR / "adapter_review_template.json").write_text(json.dumps(a_template, indent=1) + "\n", encoding="utf-8")
    print(f"oracles: {len(template)}  adapters: {len(a_template)}  -> {REVIEW_DIR}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
