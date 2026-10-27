#!/usr/bin/env python3
"""
evaluate.py - Evaluation pipeline, Milestone 1.

Builds an inventory of every planned task x model x language combination from
the generation ledgers, then runs gate 1 on each saved artifact:

  1. artifact integrity  (empty source, truncated response, Markdown residue)
  2. syntax              (Python: ast/compile; Java: parse-only javac API)
  3. build               (Java: javac; C/C++: clang via zig, Linux target,
                          compile to object only, no linking)

Generation run folders are only read. Generated code is never executed: every
check is a parser or compiler invocation on a copy in a temporary directory,
with a timeout, a reduced environment, and annotation processing disabled.
Passing gate 1 says nothing about functional correctness.

Usage:
  python evaluate.py --config eval_config_benchmark_40.json --out results/NAME
  python evaluate.py --config ... --out ... --tasks 0,3 --workers 4
"""

import argparse
import ast
import csv
import datetime
import hashlib
import json
import math
import os
import re
import shutil
import subprocess
import sys
import tempfile
from collections import Counter, defaultdict
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

EVALUATOR_VERSION = "m1.0"
EVAL_DIR = Path(__file__).resolve().parent
LANG_EXT = {"python": "py", "java": "java", "c": "c", "cpp": "cpp"}

# Inventory statuses. Only the first two have an artifact to evaluate.
GENERATED = "generated"
GENERATED_TRUNCATED = "generated_truncated"
FAILED = "failed"                # all billed attempts used, no accepted source
INCOMPLETE = "incomplete"        # attempted, still has attempts left (in progress or stopped)
NOT_ATTEMPTED = "not_attempted"
SOURCE_MISSING = "source_missing"  # ledger says success but the file is absent
HAS_ARTIFACT = {GENERATED, GENERATED_TRUNCATED}
FINAL_STATUSES = {GENERATED, GENERATED_TRUNCATED, FAILED, SOURCE_MISSING}

# Gate 1 outcomes, in order of precedence.
GATE1_STATUSES = ["pass", "fail_integrity", "fail_syntax", "fail_compile",
                  "blocked_dependency", "toolchain_error", "no_artifact"]

INVENTORY_FIELDS = [
    "combo_index", "task_id", "prompt_title", "language", "model_label", "model_id",
    "status", "attempts_total", "billed_attempts", "success_attempt", "finish_reason",
    "truncated", "validation_status", "last_error", "run_id", "source_path",
    "source_sha256", "source_bytes", "source_lines", "raw_response_path",
    "output_tokens", "cost_usd_total",
]
GATE1_FIELDS = [
    "combo_index", "task_id", "language", "model_label", "inventory_status",
    "gate1_status", "integrity_failures", "integrity_warnings", "has_entrypoint",
    "syntax_status", "syntax_message", "build_status", "build_error_count",
    "build_warning_count", "missing_dependencies", "third_party_imports",
    "first_error", "java_file_name", "source_sha256", "diagnostics_path",
]


# ─────────────────────────────────────────────────────────────────────────────
#  CONFIG AND PLAN
# ─────────────────────────────────────────────────────────────────────────────

def load_eval_config(path: Path) -> dict:
    """Load the evaluation config and resolve its paths relative to the file."""
    path = Path(path).resolve()
    cfg = json.loads(path.read_text(encoding="utf-8"))
    base = path.parent
    cfg["_path"] = path
    cfg["project_root"] = (base / cfg.get("project_root", "..")).resolve()
    cfg["plan_config"] = (base / cfg["plan_config"]).resolve()
    cfg["runner_dir"] = (base / cfg.get("runner_dir", "../runner")).resolve()
    cfg["runs"] = [(base / run).resolve() for run in cfg["runs"]]
    cfg["toolchain_dir"] = (base / cfg.get("toolchain_dir", ".toolchain")).resolve()
    return cfg


def load_plan(cfg: dict) -> dict:
    """Read the planned tasks, models and languages from the generation config."""
    plan_cfg = json.loads(cfg["plan_config"].read_text(encoding="utf-8"))
    selection = plan_cfg["task_selection"]
    if selection.get("mode") == "orig_id_list":
        task_ids = sorted(set(selection["orig_task_ids"]))
    elif selection.get("mode") == "list":
        task_ids = sorted(set(selection["task_ids"]))
    else:
        raise ValueError(f"Unsupported task_selection mode for evaluation: {selection.get('mode')!r}")
    titles = {}
    for rel_dir in plan_cfg.get("dataset_dirs", []):
        for jf in sorted((cfg["runner_dir"] / rel_dir).resolve().glob("*.json")):
            task = json.loads(jf.read_text(encoding="utf-8"))["tasks"][0]
            titles[task["task_number"]] = task.get("prompt_title", "")
    missing = [t for t in task_ids if t not in titles]
    if missing:
        raise ValueError(f"Planned task IDs not found in the dataset: {missing}")
    models = [{"label": m.get("label", m["model_id"]), "model_id": m["model_id"]}
              for m in plan_cfg["models"]]
    return {
        "task_ids": task_ids,
        "titles": titles,
        "languages": list(plan_cfg["languages"]),
        "models": models,
        "max_attempts": int(plan_cfg.get("max_attempts", 3)),
        "expected_combinations": plan_cfg.get("expected_combinations"),
    }


def planned_combinations(plan: dict):
    """Yield (combo_index, task_id, language, model) in the runner's order:
    task -> language -> model, 1-based."""
    index = 0
    for task_id in plan["task_ids"]:
        for language in plan["languages"]:
            for model in plan["models"]:
                index += 1
                yield index, task_id, language, model


# ─────────────────────────────────────────────────────────────────────────────
#  INVENTORY
# ─────────────────────────────────────────────────────────────────────────────

def sha256_file(path: Path) -> str:
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def rel(path: Path, root: Path) -> str:
    try:
        return Path(path).resolve().relative_to(root).as_posix()
    except ValueError:
        return Path(path).as_posix()


def read_ledger(path: Path) -> tuple[list[dict], int]:
    """Read a JSONL ledger. A final line that is not valid JSON is skipped and
    counted, because an active run may be mid-write; any other bad line is an error."""
    lines = path.read_text(encoding="utf-8").splitlines()
    rows, partial = [], 0
    for line_no, line in enumerate(lines, start=1):
        if not line.strip():
            continue
        try:
            rows.append(json.loads(line))
        except json.JSONDecodeError:
            if line_no == len(lines):
                partial += 1
                continue
            raise ValueError(f"{path}:{line_no} is not valid JSON")
    return rows, partial


def row_billing(row: dict) -> str:
    """Billing class of a ledger row; mirrors runner/run_experiment.py:_row_billing
    (duplicated so the evaluator never imports the runner and its .env loading)."""
    if row.get("billing"):
        return row["billing"]
    if row.get("response_id") or row.get("cost_usd") is not None:
        return "billed"
    error = str(row.get("error") or "")
    if error.startswith("OpenRouter HTTP 429"):
        return "not_billed"
    if error.startswith("OPENROUTER_API_KEY environment variable is not set"):
        return "not_sent"
    return "unknown"


def raw_path_for(run_dir: Path, row: dict) -> Path:
    """Raw response file of a ledger row, re-rooted at run_dir (ledger paths are absolute)."""
    raw_path = Path(str(row["raw_response_path"]).replace("\\", "/"))
    return run_dir / raw_path.parent.parent.name / raw_path.parent.name / raw_path.name


def source_path_for(run_dir: Path, row: dict) -> Path:
    """Source file the runner saves for a success row (same rule as the runner's resume)."""
    raw_path = raw_path_for(run_dir, row)
    stem = re.sub(r"_attempt_\d+_response\.json$", "", raw_path.name)
    return raw_path.parent / f"{stem}.{LANG_EXT[row['language']]}"


def build_inventory(cfg: dict, plan: dict, task_filter: set | None = None) -> dict:
    root = cfg["project_root"]
    by_key: dict[tuple, list] = defaultdict(list)
    inputs, partial_total = [], 0
    for run_dir in cfg["runs"]:
        ledger = run_dir / "cost_ledger.jsonl"
        if not ledger.is_file():
            raise ValueError(f"No cost_ledger.jsonl in {run_dir}")
        rows, partial = read_ledger(ledger)
        partial_total += partial
        inputs.append({
            "run_dir": rel(run_dir, root),
            "ledger_rows": len(rows),
            "ledger_partial_lines": partial,
            "ledger_sha256": sha256_file(ledger),
            "run_summary_present": (run_dir / "run_summary.json").is_file(),
            "runner_lock_present": (run_dir.parent / ".runner.lock").is_file(),
        })
        for order, row in enumerate(rows):
            key = (int(row["task_id"]), row["language"], row["model_label"])
            by_key[key].append((run_dir, order, row))

    planned_keys = set()
    out_rows, referenced = [], set()
    duplicate_successes = []
    for index, task_id, language, model in planned_combinations(plan):
        key = (task_id, language, model["label"])
        planned_keys.add(key)
        if task_filter is not None and task_id not in task_filter:
            continue
        attempts = by_key.get(key, [])
        successes = [(d, r) for d, _, r in attempts if r.get("response_status") == "success"]
        billed = sum(1 for _, _, r in attempts if row_billing(r) == "billed")
        rec = {f: "" for f in INVENTORY_FIELDS}
        rec.update({
            "combo_index": index, "task_id": task_id, "prompt_title": plan["titles"][task_id],
            "language": language, "model_label": model["label"], "model_id": model["model_id"],
            "attempts_total": len(attempts), "billed_attempts": billed,
            "cost_usd_total": round(sum(float(r.get("cost_usd") or 0.0) for _, _, r in attempts), 8),
            "truncated": False,
        })
        if attempts:
            last = attempts[-1][2]
            rec["last_error"] = (last.get("error") or "")[:300]
            rec["validation_status"] = last.get("validation_status") or ""
        if len(successes) > 1:
            duplicate_successes.append(index)
        if successes:
            run_dir, row = successes[0]
            src = source_path_for(run_dir, row)
            rec.update({
                "success_attempt": row.get("attempt"), "finish_reason": row.get("finish_reason") or "",
                "truncated": row.get("finish_reason") == "length",
                "validation_status": row.get("validation_status") or "",
                "run_id": run_dir.name, "source_path": rel(src, root),
                "raw_response_path": rel(raw_path_for(run_dir, row), root),
                "output_tokens": row.get("output_tokens"), "last_error": "",
            })
            if src.is_file():
                data = src.read_bytes()
                referenced.add(src.resolve())
                rec.update({
                    "status": GENERATED_TRUNCATED if rec["truncated"] else GENERATED,
                    "source_sha256": hashlib.sha256(data).hexdigest(),
                    "source_bytes": len(data),
                    "source_lines": data.decode("utf-8", errors="replace").count("\n") + 1,
                })
            else:
                rec["status"] = SOURCE_MISSING
        elif not attempts:
            rec["status"] = NOT_ATTEMPTED
        elif billed >= plan["max_attempts"]:
            rec["status"] = FAILED
        else:
            rec["status"] = INCOMPLETE
        out_rows.append(rec)

    unplanned = sorted({f"{k[0]}/{k[1]}/{k[2]}" for k in by_key if k not in planned_keys})
    orphans = []
    for run_dir in cfg["runs"]:
        for src in sorted(run_dir.glob("Task_*/*/*")):
            if src.suffix.lstrip(".") in LANG_EXT.values() and src.resolve() not in referenced:
                m = re.match(r"Task(\d+)_", src.name)
                if task_filter is None or (m and int(m.group(1)) in task_filter):
                    orphans.append(rel(src, root))
    meta = {
        "planned_combinations": len(planned_keys),
        "expected_combinations": plan["expected_combinations"],
        "rows_in_inventory": len(out_rows),
        "task_filter": sorted(task_filter) if task_filter is not None else None,
        "inputs": inputs,
        "generation_active": any(i["runner_lock_present"] for i in inputs),
        "ledger_partial_lines": partial_total,
        "status_counts": dict(sorted(Counter(r["status"] for r in out_rows).items())),
        "duplicate_success_combos": duplicate_successes,
        "unplanned_ledger_keys": unplanned,
        "orphan_sources": orphans,
    }
    return {"meta": meta, "rows": out_rows}


# ─────────────────────────────────────────────────────────────────────────────
#  GATE 1: INTEGRITY
# ─────────────────────────────────────────────────────────────────────────────

_RAW_STRING_START = re.compile(r'R"([^()\\\s]{0,16})\(')


def code_only(source: str, language: str) -> str:
    """Blank out comments and string/char literals (keeping newlines) so that
    structural checks and regexes only see code. Java/C/C++ only."""
    out, i, n = [], 0, len(source)
    while i < n:
        ch = source[i]
        nxt = source[i + 1] if i + 1 < n else ""
        if ch == "/" and nxt == "/":
            j = source.find("\n", i)
            j = n if j == -1 else j
            out.append(" " * (j - i)); i = j
        elif ch == "/" and nxt == "*":
            j = source.find("*/", i + 2)
            j = n if j == -1 else j + 2
            out.append(re.sub(r"[^\n]", " ", source[i:j])); i = j
        elif language == "java" and source.startswith('"""', i):
            j = source.find('"""', i + 3)
            j = n if j == -1 else j + 3
            out.append(re.sub(r"[^\n]", " ", source[i:j])); i = j
        elif language == "cpp" and ch == "R" and _RAW_STRING_START.match(source, i):
            delim = _RAW_STRING_START.match(source, i).group(1)
            j = source.find(f'){delim}"', i)
            j = n if j == -1 else j + len(delim) + 2
            out.append(re.sub(r"[^\n]", " ", source[i:j])); i = j
        elif ch in "\"'":
            j = i + 1
            while j < n and source[j] != ch and source[j] != "\n":
                j += 2 if source[j] == "\\" else 1
            j = min(j + 1, n)
            out.append(re.sub(r"[^\n]", " ", source[i:j])); i = j
        else:
            out.append(ch); i += 1
    return "".join(out)


def delimiters_balanced(code: str) -> bool:
    pairs = {")": "(", "]": "[", "}": "{"}
    stack = []
    for ch in code:
        if ch in "([{":
            stack.append(ch)
        elif ch in pairs:
            if not stack or stack.pop() != pairs[ch]:
                return False
    return not stack


_ENTRYPOINT = {
    "python": re.compile(r"""^if\s+__name__\s*==\s*['"]__main__['"]\s*:""", re.M),
    "java": re.compile(r"\bstatic\s+(?:final\s+)?void\s+main\s*\("),
    "c": re.compile(r"\bint\s+main\s*\("),
    "cpp": re.compile(r"\bint\s+main\s*\("),
}
_PLACEHOLDER = re.compile(r"(?i)\b(?:your code here|implement (?:this|me)|not implemented)\b")


def check_integrity(source: str, language: str, truncated: bool) -> dict:
    failures, warnings = [], []
    if not source.strip():
        failures.append("empty_source")
    if truncated:
        failures.append("truncated_finish_length")
    if re.search(r"^\s*```", source, re.M):
        failures.append("markdown_fence_residue")
    code = source if language == "python" else code_only(source, language)
    if language != "python" and source.strip() and not delimiters_balanced(code):
        warnings.append("unbalanced_delimiters")
    if _PLACEHOLDER.search(source):
        warnings.append("placeholder_marker")
    return {"failures": failures, "warnings": warnings,
            "has_entrypoint": bool(_ENTRYPOINT[language].search(code)), "code": code}


# ─────────────────────────────────────────────────────────────────────────────
#  GATE 1: SYNTAX AND BUILD
# ─────────────────────────────────────────────────────────────────────────────

_ENV_KEEP = ("SYSTEMROOT", "SystemRoot", "WINDIR", "PATH", "Path", "TEMP", "TMP", "JAVA_HOME",
             "PATHEXT", "COMSPEC", "LOCALAPPDATA", "HOME", "USERPROFILE")


def tool_env(toolchain_dir: Path) -> dict:
    """Reduced environment: no API keys, no CLASSPATH/JAVA options, no include paths."""
    env = {k: v for k, v in os.environ.items() if k in _ENV_KEEP}
    cache = str(toolchain_dir / "zig-cache")
    env.update({"ZIG_GLOBAL_CACHE_DIR": cache, "ZIG_LOCAL_CACHE_DIR": cache,
                "LC_ALL": "C", "LANG": "C"})
    return env


def run_tool(cmd: list, cwd: Path, env: dict, timeout: int) -> dict:
    try:
        proc = subprocess.run(cmd, cwd=cwd, env=env, capture_output=True, timeout=timeout,
                              stdin=subprocess.DEVNULL)
    except subprocess.TimeoutExpired:
        return {"returncode": None, "output": "", "timeout": True}
    out = (proc.stdout + proc.stderr).decode("utf-8", errors="replace").replace("\r\n", "\n").replace("\r", "\n")
    return {"returncode": proc.returncode, "output": out, "timeout": False}


def check_python(source: str, filename: str) -> dict:
    """Parse and byte-compile only; compile() of a string never runs it."""
    try:
        tree = ast.parse(source, filename)
        compile(tree, filename, "exec", dont_inherit=True)
    except SyntaxError as exc:
        return {"syntax_status": "error", "syntax_message": f"line {exc.lineno}: {exc.msg}",
                "build_status": "error", "build_error_count": 1, "third_party_imports": []}
    except ValueError as exc:  # e.g. null bytes
        return {"syntax_status": "error", "syntax_message": str(exc),
                "build_status": "error", "build_error_count": 1, "third_party_imports": []}
    names = set()
    for node in ast.walk(tree):
        if isinstance(node, ast.Import):
            names.update(a.name.split(".")[0] for a in node.names)
        elif isinstance(node, ast.ImportFrom) and node.level == 0 and node.module:
            names.add(node.module.split(".")[0])
    third = sorted(n for n in names if n not in sys.stdlib_module_names and n != "__future__")
    return {"syntax_status": "ok", "syntax_message": "", "build_status": "ok",
            "build_error_count": 0, "third_party_imports": third}


_JAVA_PUBLIC_TYPE = re.compile(
    r"^\s*public\s+(?:(?:final|abstract|sealed|non-sealed|strictfp|static)\s+)*"
    r"(?:class|interface|enum|record|@interface)\s+([A-Za-z_$][\w$]*)", re.M)
_JAVAC_DIAG = re.compile(r"^[^\s:][^:]*:(\d+):(\d+): compiler\.(err|warn)\.([\w.]+)(?::\s*(.*))?$", re.M)
_CLANG_DIAG = re.compile(r"^[^\s:][^:]*:(\d+):(\d+): (fatal error|error|warning): (.*)$", re.M)
_MISSING_HEADER = re.compile(r"'([^']+)' file not found")


def java_file_name(code: str, default: str) -> str:
    """javac requires a public top-level type to live in a file of the same name."""
    match = _JAVA_PUBLIC_TYPE.search(code)
    return f"{match.group(1)}.java" if match else default


def check_java(source: str, code: str, default_name: str, tc: dict) -> dict:
    name = java_file_name(code, default_name)
    with tempfile.TemporaryDirectory(prefix="eval_java_") as tmp:
        tmp = Path(tmp)
        (tmp / "src").mkdir(); (tmp / "classes").mkdir(); (tmp / "empty").mkdir()
        (tmp / "src" / name).write_text(source, encoding="utf-8")
        cmd = [tc["javac"], "-J-Duser.language=en", "-XDrawDiagnostics", "-proc:none",
               "-implicit:none", "-encoding", "UTF-8", "-Xmaxerrs", "100", "-Xlint:none",
               "-cp", str(tmp / "empty"), "-sourcepath", str(tmp / "empty"),
               "-d", str(tmp / "classes"), name]
        res = run_tool(cmd, tmp / "src", tc["env"], tc["timeout"])
        output = res["output"].replace(str(tmp), "<tmp>")
    if res["timeout"]:
        return {"build_status": "toolchain_error", "first_error": "javac timeout",
                "diagnostics": output, "java_file_name": name}
    diags = [m for m in _JAVAC_DIAG.finditer(output)]
    errors = [m for m in diags if m.group(3) == "err"]
    warnings = [m for m in diags if m.group(3) == "warn"]
    missing = sorted({(m.group(5) or "").strip() for m in errors if m.group(4) == "doesnt.exist"})
    if res["returncode"] == 0:
        status = "ok"
    elif not errors:
        status = "toolchain_error"
    else:
        status = "blocked_dependency" if missing else "error"
    first = errors[0] if errors else None
    return {
        "build_status": status, "build_error_count": len(errors), "build_warning_count": len(warnings),
        "missing_dependencies": missing, "java_file_name": name, "diagnostics": output,
        "first_error": f"line {first.group(1)}: {first.group(4)} {first.group(5) or ''}".strip() if first else
                       ("" if status == "ok" else output.strip()[:300]),
    }


def check_c_family(source: str, language: str, filename: str, tc: dict) -> dict:
    std = tc["c_std"] if language == "c" else tc["cpp_std"]
    driver = "cc" if language == "c" else "c++"
    with tempfile.TemporaryDirectory(prefix=f"eval_{language}_") as tmp:
        tmp = Path(tmp)
        (tmp / filename).write_text(source, encoding="utf-8")
        cmd = [tc["zig"], driver, "-target", tc["target"], f"-std={std}", "-fno-color-diagnostics",
               "-fno-caret-diagnostics", "-ferror-limit=50", "-Wall", "-c", filename, "-o", "out.o"]
        res = run_tool(cmd, tmp, tc["env"], tc["timeout"])
        output = res["output"].replace(str(tmp), "<tmp>")
    if res["timeout"]:
        return {"build_status": "toolchain_error", "first_error": "compiler timeout", "diagnostics": output}
    diags = list(_CLANG_DIAG.finditer(output))
    errors = [m for m in diags if m.group(3) in ("error", "fatal error")]
    warnings = [m for m in diags if m.group(3) == "warning"]
    missing = sorted({h.group(1) for m in errors for h in [_MISSING_HEADER.search(m.group(4))] if h})
    if res["returncode"] == 0:
        status = "ok"
    elif not errors:
        status = "toolchain_error"
    else:
        status = "blocked_dependency" if missing else "error"
    first = errors[0] if errors else None
    return {
        "build_status": status, "build_error_count": len(errors), "build_warning_count": len(warnings),
        "missing_dependencies": missing, "diagnostics": output,
        "first_error": f"line {first.group(1)}: {first.group(4)}" if first else
                       ("" if status == "ok" else output.strip()[:300]),
    }


def java_parse_batch(paths: list[Path], tc: dict) -> dict:
    """Parse-only syntax check for many Java files with the helper (one JVM per batch)."""
    results = {}
    for start in range(0, len(paths), 100):
        batch = [str(p) for p in paths[start:start + 100]]
        res = run_tool([tc["java"], "-Duser.language=en", "-cp", str(tc["java_helper_dir"]),
                        "JavaParseCheck", *batch], EVAL_DIR, tc["env"], tc["timeout"] * 4)
        if res["timeout"] or res["returncode"] != 0:
            raise RuntimeError(f"JavaParseCheck failed: {res['output'][:500]}")
        for line in res["output"].splitlines():
            parts = line.split("\t")
            if len(parts) == 4:
                results[str(Path(parts[0]).resolve())] = {
                    "syntax_status": parts[1], "syntax_message": parts[3]}
    return results


def gate1_status(integrity: dict, result: dict) -> str:
    if integrity["failures"]:
        return "fail_integrity"
    if result.get("build_status") == "toolchain_error":
        return "toolchain_error"
    if result.get("syntax_status") == "error":
        return "fail_syntax"
    if result.get("build_status") == "blocked_dependency":
        return "blocked_dependency"
    if result.get("build_status") == "error":
        return "fail_compile"
    return "pass"


def evaluate_artifact(rec: dict, root: Path, tc: dict, java_syntax: dict, diag_dir: Path) -> dict:
    out = {f: "" for f in GATE1_FIELDS}
    out.update({k: rec[k] for k in ("combo_index", "task_id", "language", "model_label")})
    out["inventory_status"] = rec["status"]
    if rec["status"] not in HAS_ARTIFACT:
        out["gate1_status"] = "no_artifact"
        return out
    src_path = root / rec["source_path"]
    source = src_path.read_text(encoding="utf-8", errors="replace")
    language = rec["language"]
    integrity = check_integrity(source, language, rec["truncated"])
    if language == "python":
        result = check_python(source, src_path.name)
    elif language == "java":
        result = dict(java_syntax.get(str(src_path.resolve()), {"syntax_status": "not_checked", "syntax_message": ""}))
        result.update(check_java(source, integrity["code"], src_path.name, tc))
    else:
        result = {"syntax_status": "n/a", "syntax_message": ""}
        result.update(check_c_family(source, language, src_path.name, tc))
    out.update({
        "gate1_status": gate1_status(integrity, result),
        "integrity_failures": ";".join(integrity["failures"]),
        "integrity_warnings": ";".join(integrity["warnings"]),
        "has_entrypoint": integrity["has_entrypoint"],
        "syntax_status": result.get("syntax_status", ""),
        "syntax_message": result.get("syntax_message", ""),
        "build_status": result.get("build_status", ""),
        "build_error_count": result.get("build_error_count", 0),
        "build_warning_count": result.get("build_warning_count", 0),
        "missing_dependencies": ";".join(result.get("missing_dependencies", [])),
        "third_party_imports": ";".join(result.get("third_party_imports", [])),
        "first_error": " ".join((result.get("first_error") or result.get("syntax_message") or "").split())[:300],
        "java_file_name": result.get("java_file_name", ""),
        "source_sha256": sha256_file(src_path),
    })
    if result.get("diagnostics", "").strip():
        diag = diag_dir / f"{rec['combo_index']:04d}_Task{rec['task_id']}_{rec['model_label']}_{language}.txt"
        diag.write_text(result["diagnostics"], encoding="utf-8", newline="\n")
        out["diagnostics_path"] = diag.relative_to(diag_dir.parent).as_posix()
    return out


def prepare_toolchain(cfg: dict) -> dict:
    """Locate tools, compile the Java parse helper, and record versions."""
    tdir = cfg["toolchain_dir"]
    tool_cfg = cfg.get("toolchain", {})
    env = tool_env(tdir)
    zig = None
    for candidate in sorted(tdir.glob("Lib/site-packages/ziglang/zig*")) + sorted(tdir.glob("lib/python*/site-packages/ziglang/zig")):
        if candidate.is_file() and candidate.stem == "zig":
            zig = str(candidate)
    javac, java = shutil.which("javac"), shutil.which("java")
    if not (zig and javac and java):
        raise RuntimeError(f"Toolchain incomplete: zig={zig}, javac={javac}, java={java}. See evaluation/README.md.")
    helper_src = EVAL_DIR / "tools" / "JavaParseCheck.java"
    helper_dir = tdir / "java-helper" / sha256_file(helper_src)[:12]
    if not (helper_dir / "JavaParseCheck.class").is_file():
        helper_dir.mkdir(parents=True, exist_ok=True)
        res = run_tool([javac, "-d", str(helper_dir), str(helper_src)], EVAL_DIR, env, 300)
        if res["returncode"] != 0:
            raise RuntimeError(f"Could not compile JavaParseCheck: {res['output'][:500]}")

    def version(cmd):
        res = run_tool(cmd, EVAL_DIR, env, 300)
        return (res["output"].strip().splitlines() or ["unknown"])[0]

    return {
        "zig": zig, "javac": javac, "java": java, "java_helper_dir": helper_dir, "env": env,
        "target": tool_cfg.get("target", "x86_64-linux-gnu"),
        "c_std": tool_cfg.get("c_std", "gnu17"), "cpp_std": tool_cfg.get("cpp_std", "gnu++17"),
        "timeout": int(tool_cfg.get("timeout_seconds", 120)),
        "versions": {
            "python": sys.version.split()[0],
            "javac": version([javac, "-version"]),
            "java": version([java, "-version"]),
            "zig": version([zig, "version"]),
            "clang": version([zig, "cc", "--version"]),
        },
    }


def run_gate1(inventory: dict, cfg: dict, tc: dict, out_dir: Path, workers: int) -> list[dict]:
    root = cfg["project_root"]
    diag_dir = out_dir / "diagnostics"
    diag_dir.mkdir(parents=True, exist_ok=True)
    rows = inventory["rows"]
    java_paths = [root / r["source_path"] for r in rows if r["language"] == "java" and r["status"] in HAS_ARTIFACT]
    java_syntax = java_parse_batch(java_paths, tc) if java_paths else {}
    # Warm the zig cache once so parallel first compiles do not all rebuild it.
    if any(r["language"] in ("c", "cpp") and r["status"] in HAS_ARTIFACT for r in rows):
        for lang, name in (("c", "w.c"), ("cpp", "w.cpp")):
            check_c_family("int f(void){return 0;}\n", lang, name, tc)
    with ThreadPoolExecutor(max_workers=max(1, workers)) as pool:
        results = list(pool.map(lambda r: evaluate_artifact(r, root, tc, java_syntax, diag_dir), rows))
    return sorted(results, key=lambda r: r["combo_index"])


# ─────────────────────────────────────────────────────────────────────────────
#  SUMMARY STATISTICS
# ─────────────────────────────────────────────────────────────────────────────

def wilson(k: int, n: int, z: float = 1.959963984540054) -> list:
    """Wilson score 95% interval for a proportion; [None, None] when n == 0."""
    if n == 0:
        return [None, None]
    p = k / n
    denom = 1 + z * z / n
    centre = (p + z * z / (2 * n)) / denom
    half = z * math.sqrt(p * (1 - p) / n + z * z / (4 * n * n)) / denom
    return [round(max(0.0, centre - half), 4), round(min(1.0, centre + half), 4)]


def mcnemar_exact(b: int, c: int) -> float:
    """Two-sided exact McNemar p-value from the discordant counts b and c."""
    n = b + c
    if n == 0:
        return 1.0
    tail = sum(math.comb(n, i) for i in range(0, min(b, c) + 1)) / 2 ** n
    return round(min(1.0, 2 * tail), 6)


def rate(k: int, n: int) -> dict:
    return {"k": k, "n": n, "rate": round(k / n, 4) if n else None, "ci95_wilson": wilson(k, n)}


def summarize(inventory: dict, gate1: list[dict], plan: dict) -> dict:
    inv = {r["combo_index"]: r for r in inventory["rows"]}

    def group_stats(rows):
        counts = Counter(r["gate1_status"] for r in rows)
        n_planned = len(rows)
        n_artifacts = sum(1 for r in rows if r["inventory_status"] in HAS_ARTIFACT)
        n_final = sum(1 for r in rows if r["inventory_status"] in FINAL_STATUSES)
        n_evaluable = n_artifacts - counts["blocked_dependency"] - counts["toolchain_error"]
        return {
            "planned": n_planned,
            "generation_final": n_final,
            "artifacts": n_artifacts,
            "gate1_counts": {s: counts.get(s, 0) for s in GATE1_STATUSES},
            "pass_over_final": rate(counts["pass"], n_final),
            "pass_over_evaluable_artifacts": rate(counts["pass"], n_evaluable),
        }

    by = lambda key: {k: group_stats([r for r in gate1 if r[key] == k])
                      for k in sorted({r[key] for r in gate1})}
    by_cell = defaultdict(dict)
    for model in sorted({r["model_label"] for r in gate1}):
        for lang in plan["languages"]:
            rows = [r for r in gate1 if r["model_label"] == model and r["language"] == lang]
            if rows:
                by_cell[model][lang] = group_stats(rows)

    # Paired comparison on (task, language) cells where every model's generation
    # is final. Outcome = gate 1 pass; a failed generation counts as not passing.
    cells = defaultdict(dict)
    for r in gate1:
        cells[(r["task_id"], r["language"])][r["model_label"]] = r
    models = [m["label"] for m in plan["models"]]
    complete = [c for c, ms in cells.items()
                if all(m in ms and inv[ms[m]["combo_index"]]["status"] in FINAL_STATUSES for m in models)]
    pairs = []
    for i, a in enumerate(models):
        for b in models[i + 1:]:
            only_a = sum(1 for c in complete if cells[c][a]["gate1_status"] == "pass" and cells[c][b]["gate1_status"] != "pass")
            only_b = sum(1 for c in complete if cells[c][b]["gate1_status"] == "pass" and cells[c][a]["gate1_status"] != "pass")
            pairs.append({"model_a": a, "model_b": b, "a_pass_b_not": only_a, "b_pass_a_not": only_b,
                          "mcnemar_exact_p_unadjusted": mcnemar_exact(only_a, only_b)})
    return {
        "overall": group_stats(gate1),
        "by_model": by("model_label"),
        "by_language": by("language"),
        "by_model_language": by_cell,
        "paired": {"complete_cells": len(complete), "total_cells": len(cells),
                   "outcome": "gate1_status == pass (failed generation counts as not pass)",
                   "pairs": pairs},
        "integrity_failure_counts": dict(sorted(Counter(
            f for r in gate1 for f in r["integrity_failures"].split(";") if f).items())),
        "missing_dependency_counts": dict(sorted(Counter(
            d for r in gate1 for d in r["missing_dependencies"].split(";") if d).items())),
        "python_third_party_import_counts": dict(sorted(Counter(
            d for r in gate1 for d in r["third_party_imports"].split(";") if d).items())),
        "truncated_but_built_ok": sum(1 for r in gate1 if "truncated_finish_length" in r["integrity_failures"]
                                      and r["build_status"] == "ok"),
    }


def fmt_rate(r: dict) -> str:
    if not r["n"]:
        return "n/a"
    lo, hi = r["ci95_wilson"]
    return f"{r['k']}/{r['n']} = {100 * r['rate']:.1f}% [{100 * lo:.1f}, {100 * hi:.1f}]"


def summary_markdown(summary: dict, inventory: dict, manifest: dict) -> str:
    meta = inventory["meta"]
    o = summary["overall"]
    lines = [
        f"# Gate 1 summary: {manifest['eval_name']}",
        "",
        f"Evaluator {manifest['evaluator_version']}. Inventory snapshot of {meta['planned_combinations']} planned combinations"
        + (f", filtered to tasks {meta['task_filter']}" if meta["task_filter"] else "") + ".",
    ]
    if meta["generation_active"]:
        lines.append("**Generation was still running when this snapshot was taken: counts are provisional.**")
    lines += [
        "",
        "## Inventory",
        "",
        "| status | count |", "|---|---|",
        *[f"| {k} | {v} |" for k, v in meta["status_counts"].items()],
        "",
        "## Gate 1 funnel (all rows)",
        "",
        "| gate 1 status | count |", "|---|---|",
        *[f"| {k} | {v} |" for k, v in o["gate1_counts"].items()],
        "",
        f"- Pass over generation-final combinations: {fmt_rate(o['pass_over_final'])}",
        f"- Pass over evaluable artifacts (excludes dependency-blocked and toolchain errors): {fmt_rate(o['pass_over_evaluable_artifacts'])}",
        f"- Truncated outputs that still compiled (counted as fail_integrity): {summary['truncated_but_built_ok']}",
        "",
        "## By model and language (pass over generation-final, Wilson 95% CI)",
        "",
        "| model | " + " | ".join(manifest["languages"]) + " |",
        "|---|" + "---|" * len(manifest["languages"]),
    ]
    for model, langs in summary["by_model_language"].items():
        lines.append(f"| {model} | " + " | ".join(fmt_rate(langs[l]["pass_over_final"]) if l in langs else "n/a"
                                                 for l in manifest["languages"]) + " |")
    p = summary["paired"]
    lines += [
        "",
        f"## Paired comparison ({p['complete_cells']} of {p['total_cells']} task-language cells complete for every model)",
        "",
        "Exact McNemar, unadjusted for multiple comparisons; exploratory.",
        "",
        "| model A | model B | A pass, B not | B pass, A not | p |", "|---|---|---|---|---|",
        *[f"| {x['model_a']} | {x['model_b']} | {x['a_pass_b_not']} | {x['b_pass_a_not']} | {x['mcnemar_exact_p_unadjusted']} |"
          for x in p["pairs"]],
        "",
        "## Dependencies and integrity",
        "",
        f"- Integrity failures: {summary['integrity_failure_counts'] or 'none'}",
        f"- Missing third-party headers/packages (C, C++, Java): {summary['missing_dependency_counts'] or 'none'}",
        f"- Python third-party imports (not resolved at this gate): {summary['python_third_party_import_counts'] or 'none'}",
        "",
        "## Limits of this gate",
        "",
        "- Passing gate 1 means the file is complete and compiles; it is not evidence of functional correctness.",
        "- Python is checked for syntax only (ast + compile); imports are not resolved.",
        f"- C/C++ are compiled as single translation units for {manifest['toolchain']['target']} "
        f"({manifest['toolchain']['c_std']}/{manifest['toolchain']['cpp_std']}), without linking.",
        "- Java is compiled with javac and no third-party classpath; missing packages are reported as blocked_dependency.",
        "- Generated code was not executed.",
    ]
    return "\n".join(lines) + "\n"


# ─────────────────────────────────────────────────────────────────────────────
#  OUTPUT
# ─────────────────────────────────────────────────────────────────────────────

def write_csv(path: Path, fields: list, rows: list[dict]) -> None:
    with path.open("w", encoding="utf-8", newline="") as fh:
        writer = csv.DictWriter(fh, fieldnames=fields, lineterminator="\n")
        writer.writeheader()
        for row in rows:
            writer.writerow({f: row.get(f, "") for f in fields})


def write_json(path: Path, data) -> None:
    path.write_text(json.dumps(data, indent=2, sort_keys=False, default=str) + "\n",
                    encoding="utf-8", newline="\n")


def git_head(root: Path) -> str:
    try:
        res = subprocess.run(["git", "rev-parse", "HEAD"], cwd=root, capture_output=True, timeout=30)
        return res.stdout.decode().strip() or "unknown"
    except (OSError, subprocess.TimeoutExpired):
        return "unknown"


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description="Evaluation Milestone 1: inventory and gate 1.")
    parser.add_argument("--config", required=True)
    parser.add_argument("--out", required=True, help="Results folder (must not already contain results).")
    parser.add_argument("--tasks", help="Comma-separated task IDs to evaluate (sampling).")
    parser.add_argument("--workers", type=int, default=4)
    parser.add_argument("--inventory-only", action="store_true")
    args = parser.parse_args(argv)

    cfg = load_eval_config(Path(args.config))
    out_dir = Path(args.out).resolve()
    for run_dir in cfg["runs"]:
        if out_dir == run_dir or run_dir in out_dir.parents or out_dir in run_dir.parents:
            raise SystemExit(f"Refusing to write results inside or above a generation run folder: {out_dir}")
    if out_dir.exists() and any(out_dir.iterdir()):
        raise SystemExit(f"Results folder already has files: {out_dir}. Choose a new --out.")
    out_dir.mkdir(parents=True, exist_ok=True)

    plan = load_plan(cfg)
    task_filter = {int(t) for t in args.tasks.split(",")} if args.tasks else None
    if task_filter and not task_filter <= set(plan["task_ids"]):
        raise SystemExit(f"Tasks not in plan: {sorted(task_filter - set(plan['task_ids']))}")
    inventory = build_inventory(cfg, plan, task_filter)
    write_json(out_dir / "inventory.json", inventory)
    write_csv(out_dir / "inventory.csv", INVENTORY_FIELDS, inventory["rows"])

    manifest = {
        "eval_name": out_dir.name,
        "evaluator_version": EVALUATOR_VERSION,
        "generated_at": datetime.datetime.now().isoformat(timespec="seconds"),
        "git_head": git_head(cfg["project_root"]),
        "evaluator_sha256": sha256_file(Path(__file__)),
        "java_helper_sha256": sha256_file(EVAL_DIR / "tools" / "JavaParseCheck.java"),
        "config_path": rel(cfg["_path"], cfg["project_root"]),
        "plan_config": rel(cfg["plan_config"], cfg["project_root"]),
        "plan_config_sha256": sha256_file(cfg["plan_config"]),
        "languages": plan["languages"],
        "models": plan["models"],
        "task_filter": inventory["meta"]["task_filter"],
        "inputs": inventory["meta"]["inputs"],
    }
    print(f"Inventory: {inventory['meta']['rows_in_inventory']} rows, {inventory['meta']['status_counts']}")
    if args.inventory_only:
        write_json(out_dir / "eval_manifest.json", manifest)
        return 0

    tc = prepare_toolchain(cfg)
    manifest["toolchain"] = {
        "versions": tc["versions"], "target": tc["target"], "c_std": tc["c_std"], "cpp_std": tc["cpp_std"],
        "timeout_seconds": tc["timeout"],
        "c_flags": ["-target", tc["target"], f"-std={tc['c_std']}", "-Wall", "-ferror-limit=50", "-c"],
        "cpp_flags": ["-target", tc["target"], f"-std={tc['cpp_std']}", "-Wall", "-ferror-limit=50", "-c"],
        "javac_flags": ["-XDrawDiagnostics", "-proc:none", "-implicit:none", "-encoding", "UTF-8",
                        "-Xlint:none", "-cp <empty>", "-sourcepath <empty>"],
        "python_check": "ast.parse + compile(dont_inherit=True); no import resolution",
    }
    gate1 = run_gate1(inventory, cfg, tc, out_dir, args.workers)
    write_csv(out_dir / "gate1_results.csv", GATE1_FIELDS, gate1)
    write_json(out_dir / "gate1_results.json", gate1)
    summary = summarize(inventory, gate1, plan)
    write_json(out_dir / "summary.json", summary)
    (out_dir / "summary.md").write_text(summary_markdown(summary, inventory, manifest), encoding="utf-8", newline="\n")
    write_json(out_dir / "eval_manifest.json", manifest)
    print(f"Gate 1: {summary['overall']['gate1_counts']}")
    print(f"Results: {out_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
