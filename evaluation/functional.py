#!/usr/bin/env python3
"""
functional.py - Evaluation pipeline, Milestone 2: functional correctness.

  catalog  Analyse the recovered upstream tests (evaluation/.upstream, see
           upstream_fetch.py) and write an oracle catalog: for every task x
           language, which test files carry checks, how they bind to the code
           under test, the canonical symbols they call (with signatures read
           from the original implementations), and a fingerprint of their
           test inputs/expected values. One reference test per task x language
           is proposed; it is used only after a human approves it in
           functional/oracle_review.json.
  plan     For every combination in a gate 1 results folder, pick the oracle,
           draft an adapter (static analysis only), optionally build the test
           workspace, and record the functional status. Without a sandbox every
           combination is not_testable.

Adapters only bind names: a canonical symbol from the original test is mapped
to a symbol in the generated artifact. Test inputs and expected values are
never edited; the test file is changed only at its binding lines, and every
change is recorded in the workspace manifest.

Generated code is never executed on the host. Execution goes through an
Executor; the only local executor (TrustedFixtureExecutor) refuses any
workspace whose code is not on the fixture allowlist.

Usage:
  python functional.py catalog
  python functional.py plan --gate1 results/<name> --out results/<name>-functional [--workspaces]
"""

import argparse
import ast
import csv
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET
from collections import Counter, defaultdict
from pathlib import Path

EVAL_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(EVAL_DIR))
from evaluate import code_only, java_file_name  # noqa: E402  (same lexer as gate 1)

UPSTREAM_DIR = EVAL_DIR / ".upstream"
FUNCTIONAL_DIR = EVAL_DIR / "functional"
CATALOG_PATH = FUNCTIONAL_DIR / "oracle_catalog.json"
REVIEW_PATH = FUNCTIONAL_DIR / "oracle_review.json"
ADAPTER_REVIEW_PATH = FUNCTIONAL_DIR / "adapter_review.json"

# Functional result categories. Sub-reasons are recorded separately.
PASSED, FAILED, ERROR, TIMEOUT, NOT_TESTABLE = "passed", "failed", "error", "timeout", "not_testable"
RESULT_CATEGORIES = [PASSED, FAILED, ERROR, TIMEOUT, NOT_TESTABLE]

# Order used to break ties when choosing a reference test (upstream folder names).
MODEL_ORDER = ["claude", "gpt4o", "gtp4o", "gemini", "codestral", "llama-3", "llama3"]
USABLE_BINDINGS = {"import_original", "include_original", "class_reference"}

PLAN_FIELDS = [
    "combo_index", "task_id", "language", "model_label", "gate1_status", "oracle_status",
    "oracle_original_model", "oracle_test_path", "adapter_status", "adapter_bindings",
    "functional_status", "reason", "workspace",
]


def sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def read_text(path: Path) -> str:
    return Path(path).read_text(encoding="utf-8", errors="replace")


# ─────────────────────────────────────────────────────────────────────────────
#  SOURCE ANALYSIS (regex/AST only; nothing is executed)
# ─────────────────────────────────────────────────────────────────────────────

_C_KEYWORDS = {"if", "for", "while", "switch", "return", "sizeof", "else", "do", "case", "catch"}
_C_FUNC_DEF = re.compile(
    r"^[ \t]*(?!(?:if|for|while|switch|return|else|do|case)\b)"
    r"((?:[\w:<>,\*&~\[\]]+[ \t\*&]+)+?)(~?[A-Za-z_]\w*(?:::~?[A-Za-z_]\w*)?)[ \t]*\(([^;{}()]*(?:\([^()]*\)[^;{}()]*)*)\)"
    r"[ \t\w]*(?:->[^{;]*)?\s*\{", re.M)
_JAVA_TYPE_DECL = re.compile(r"\b(?:class|interface|enum|record)\s+([A-Z_$][\w$]*)")
_JAVA_METHOD = re.compile(
    r"^[ \t]*((?:(?:public|protected|private|static|final|synchronized|abstract|native)\s+)*)"
    r"(?:<[^>]+>\s+)?([\w$<>\[\],.? ]+?)\s+([a-zA-Z_$][\w$]*)\s*\(([^)]*)\)\s*(?:throws[^{;]+)?\{", re.M)
_LITERAL = re.compile(r'"(?:[^"\\\n]|\\.)*"|\'(?:[^\'\\\n]|\\.)*\'|(?<![\w.])-?\d+(?:\.\d+)?(?![\w.])')


def split_params(text: str) -> list[str]:
    """Split a parameter list on top-level commas."""
    text = text.strip()
    if not text or text == "void":
        return []
    parts, depth, current = [], 0, ""
    for ch in text:
        if ch in "<([":
            depth += 1
        elif ch in ">)]":
            depth -= 1
        if ch == "," and depth == 0:
            parts.append(current.strip()); current = ""
        else:
            current += ch
    parts.append(current.strip())
    return [p for p in parts if p]


def c_functions(source: str) -> dict:
    """Function definitions in C/C++ source: name -> {return, params}."""
    code = code_only(source, "cpp")
    funcs = {}
    for m in _C_FUNC_DEF.finditer(code):
        name = m.group(2).split("::")[-1]
        if name in _C_KEYWORDS:
            continue
        funcs.setdefault(name, {"return": " ".join(m.group(1).split()), "params": split_params(m.group(3))})
    return funcs


def python_symbols(source: str) -> dict:
    """Top-level functions and classes (with methods) of a Python module, by AST."""
    try:
        tree = ast.parse(source)
    except SyntaxError:
        return {}
    out = {}
    for node in tree.body:
        if isinstance(node, (ast.FunctionDef, ast.AsyncFunctionDef)):
            a = node.args
            out[node.name] = {"kind": "function", "arity": len(a.posonlyargs) + len(a.args),
                              "required": len(a.posonlyargs) + len(a.args) - len(a.defaults),
                              "varargs": a.vararg is not None}
        elif isinstance(node, ast.ClassDef):
            out[node.name] = {"kind": "class", "methods": sorted(
                n.name for n in node.body if isinstance(n, (ast.FunctionDef, ast.AsyncFunctionDef)))}
    return out


def java_symbols(source: str) -> dict:
    """Types and methods of a Java file: {class: {method: [{static, return, params}]}} (regex)."""
    code = code_only(source, "java")
    classes = {}
    decls = [(m.start(), m.group(1)) for m in _JAVA_TYPE_DECL.finditer(code)]
    for m in _JAVA_METHOD.finditer(code):
        owner = None
        for pos, name in decls:
            if pos < m.start():
                owner = name
        if owner is None or m.group(3) in ("if", "for", "while", "switch", "catch"):
            continue
        classes.setdefault(owner, {}).setdefault(m.group(3), []).append({
            "static": "static" in m.group(1).split(), "return": " ".join(m.group(2).split()),
            "params": split_params(m.group(4))})
    for _, name in decls:
        classes.setdefault(name, {})
    return classes


def normalize_name(name: str) -> str:
    return re.sub(r"[^a-z0-9]", "", name.lower())


def case_fingerprint(test_text: str, binding_lines: list[str]) -> tuple[str, int]:
    """Fingerprint of the literals (inputs/expected values) a test uses, ignoring
    comments and binding lines, so tests for different models can be compared."""
    text = "\n".join(l for l in test_text.splitlines() if l.strip() not in {b.strip() for b in binding_lines})
    text = re.sub(r"(?m)^\s*(?:#|//).*$", "", text)
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    literals = sorted(_LITERAL.findall(text))
    return sha256_bytes("\x00".join(literals).encode("utf-8"))[:16], len(literals)


# ─────────────────────────────────────────────────────────────────────────────
#  ORACLE CATALOG
# ─────────────────────────────────────────────────────────────────────────────

_PY_BINDING = re.compile(r"^[ \t]*#?[ \t]*from[ \t]+(output(?:\.[\w-]+)+)[ \t]+import[ \t]+(.+?)[ \t]*$", re.M)
_C_INCLUDE_ORIGINAL = re.compile(r'^[ \t]*#[ \t]*include[ \t]+"([^"]*source/output/[^"]+)"[^\n]*$', re.M)
_CHECKS = {
    "python": re.compile(r"self\.assert\w+\(|^\s*assert\s|pytest\.raises", re.M),
    "java": re.compile(r"\bassert\w*\s*\(|\bfail\s*\("),
    "c": re.compile(r"\bassert\s*\(|\bFAIL\b"),
    "cpp": re.compile(r"\bassert\s*\(|\bFAIL\b|ASSERT_|EXPECT_"),
}


def parse_import_names(text: str) -> list[tuple[str, str]]:
    """'a as b, c' -> [(a, b), (c, c)]"""
    text = text.strip().strip("()")
    out = []
    for part in text.split(","):
        bits = part.split()
        if len(bits) == 3 and bits[1] == "as":
            out.append((bits[0], bits[2]))
        elif len(bits) == 1:
            out.append((bits[0], bits[0]))
    return out


def analyze_test(language: str, test_text: str, original_impl: str | None, own_model: str) -> dict:
    """Classify one upstream test file. original_impl is the same model's
    implementation, used only to read names and signatures."""
    info = {"checks": len(_CHECKS[language].findall(test_text)), "binding": "none",
            "binding_lines": [], "symbols": [], "framework": "", "verdict_style": "", "notes": []}
    if not test_text.strip():
        info["binding"] = "empty"
        return info
    if language == "python":
        info["framework"] = ("unittest" if "unittest.TestCase" in test_text else
                             "pytest" if re.search(r"^def test_", test_text, re.M) and "pytest" in test_text else
                             "script_asserts" if re.search(r"^\s*assert\s", test_text, re.M) else "none")
        lines = [m for m in _PY_BINDING.finditer(test_text)
                 if m.group(1).split(".")[1].replace("-", "") == own_model.replace("-", "")]
        all_lines = [m.group(0) for m in _PY_BINDING.finditer(test_text)]
        impl_syms = python_symbols(original_impl or "")
        if lines:
            info["binding"] = "import_original"
            info["binding_lines"] = all_lines
            for m in lines:
                for original, alias in parse_import_names(m.group(2)):
                    sig = impl_syms.get(original, {})
                    info["symbols"].append({"canonical": alias, "original": original, **sig})
        elif re.search(r"^def (?!test)", test_text, re.M) or re.search(r"^class (?!Test)", test_text, re.M):
            info["binding"] = "embedded_reference"
        info["verdict_style"] = "unittest" if info["framework"] == "unittest" else "exit_code"
    elif language in ("c", "cpp"):
        includes = _C_INCLUDE_ORIGINAL.findall(test_text)
        impl_funcs = c_functions(original_impl or "")
        test_funcs = c_functions(test_text)
        called = set(re.findall(r"\b([A-Za-z_]\w*)\s*\(", test_text))
        has_assert = bool(re.search(r"\bassert\s*\(", test_text))
        has_fail = bool(re.search(r"\bFAIL\b", test_text))
        info["framework"] = "+".join(x for x, ok in (("assert", has_assert), ("pass_fail_output", has_fail)) if ok) or "none"
        info["verdict_style"] = "exit_code_and_fail_lines"
        if includes:
            info["binding"] = "include_original"
            info["binding_lines"] = [m.group(0) for m in _C_INCLUDE_ORIGINAL.finditer(test_text)]
            names = sorted(n for n in called if n in impl_funcs and n not in test_funcs and n != "main")
            info["symbols"] = [{"canonical": n, "original": n, **impl_funcs[n]} for n in names]
        elif any(n in impl_funcs for n in test_funcs if n != "main"):
            info["binding"] = "embedded_reference"
            info["notes"].append("test defines its own copy of: " + ", ".join(
                sorted(n for n in test_funcs if n in impl_funcs and n != "main")))
    else:  # java
        info["framework"] = ("junit5" if "org.junit.jupiter" in test_text else
                             "junit4" if "org.junit" in test_text else "none")
        info["verdict_style"] = "junit_xml"
        pkg = re.search(r"^\s*package\s+([\w.]+)\s*;", test_text, re.M)
        info["package"] = pkg.group(1) if pkg else ""
        impl = java_symbols(original_impl or "")
        test_types = set(java_symbols(test_text))
        referenced = sorted(c for c in impl if re.search(rf"\b{re.escape(c)}\b", test_text) and c not in test_types)
        if referenced:
            info["binding"] = "class_reference"
            for cls in referenced:
                static_imports = re.findall(rf"import\s+static\s+[\w.]*\b{re.escape(cls)}\.(\w+)\s*;", test_text)
                calls = set(re.findall(rf"\b{re.escape(cls)}\s*\.\s*(\w+)\s*\(", test_text)) | set(static_imports)
                if re.search(rf"\bnew\s+{re.escape(cls)}\s*[(<]", test_text):
                    info["notes"].append(f"instantiates {cls}")
                    info["instance_use"] = True
                for method in sorted(calls):
                    for sig in impl[cls].get(method, [{}]):
                        info["symbols"].append({"canonical": f"{cls}.{method}", "class": cls, "method": method, **sig})
        elif test_types & set(impl):
            info["binding"] = "embedded_reference"
    fp, n_literals = case_fingerprint(test_text, info["binding_lines"])
    info["case_fingerprint"], info["literal_count"] = fp, n_literals
    return info


def build_catalog(manifest: dict) -> dict:
    files = manifest["files"]
    impl_by = {(f["language"], f["task_id"], f["original_model"]): f for f in files if f["role"] == "implementation"}
    entries = defaultdict(list)
    for f in files:
        if f["role"] != "test":
            continue
        impl = impl_by.get((f["language"], f["task_id"], f["original_model"]))
        impl_text = read_text(EVAL_DIR / impl["local_path"]) if impl else None
        info = analyze_test(f["language"], read_text(EVAL_DIR / f["local_path"]), impl_text, f["original_model"])
        info.update({"original_model": f["original_model"], "test_path": f["path"], "test_sha256": f["sha256"],
                     "test_local_path": f["local_path"],
                     "implementation_path": impl["path"] if impl else "",
                     "usable": info["checks"] > 0 and info["binding"] in USABLE_BINDINGS and bool(info["symbols"])})
        entries[(f["task_id"], f["language"])].append(info)
    tasks = sorted({f["task_id"] for f in files})
    catalog = {"source": manifest["repos"], "entries": {}}
    for task in tasks:
        for language in ("python", "java", "c", "cpp"):
            tests = sorted(entries.get((task, language), []),
                           key=lambda e: (MODEL_ORDER.index(e["original_model"]) if e["original_model"] in MODEL_ORDER else 99,
                                          e["test_path"]))
            usable = [t for t in tests if t["usable"]]
            clusters = Counter(t["case_fingerprint"] for t in usable)
            reference = None
            if usable:
                best = max(clusters.values())
                fp = next(t["case_fingerprint"] for t in usable if clusters[t["case_fingerprint"]] == best)
                reference = next(t for t in usable if t["case_fingerprint"] == fp)
            catalog["entries"][f"{task}/{language}"] = {
                "task_id": task, "language": language,
                "test_files": len(tests), "usable_files": len(usable),
                "reference": {k: reference[k] for k in ("original_model", "test_path", "test_sha256")} if reference else None,
                "agreement": f"{clusters[reference['case_fingerprint']]}/{len(usable)}" if reference else "0/0",
                "tests": tests,
            }
    return catalog


def load_review() -> dict:
    return json.loads(REVIEW_PATH.read_text(encoding="utf-8")) if REVIEW_PATH.is_file() else {}


def oracle_for(catalog: dict, review: dict, task_id: int, language: str) -> tuple[str, dict | None]:
    """(oracle_status, reference test info). Status: no_oracle | unreviewed | rejected | approved."""
    entry = catalog["entries"].get(f"{task_id}/{language}")
    if not entry or not entry["reference"]:
        return "no_oracle", None
    ref = next(t for t in entry["tests"] if t["test_path"] == entry["reference"]["test_path"])
    decision = review.get(f"{task_id}/{language}")
    if not decision:
        return "unreviewed", ref
    if decision.get("test_sha256") != ref["test_sha256"]:
        return "unreviewed", ref  # review was for a different file version
    return ("approved" if decision.get("approved") else "rejected"), ref


# ─────────────────────────────────────────────────────────────────────────────
#  ADAPTERS
# ─────────────────────────────────────────────────────────────────────────────

def artifact_symbols(language: str, source: str) -> dict:
    """{name: {"arity", "params", "return"}} of callable symbols in a generated artifact.
    Python has no declared types, so params/return are None there."""
    if language == "python":
        return {name: {"arity": sym.get("arity"), "params": None, "return": None}
                for name, sym in python_symbols(source).items() if sym["kind"] == "function"}
    if language in ("c", "cpp"):
        return {n: {"arity": len(f["params"]), "params": f["params"], "return": f["return"]}
                for n, f in c_functions(source).items() if n != "main"}
    return {f"{cls}.{m}": {"arity": len(sigs[0]["params"]), "params": sigs[0]["params"],
                           "return": sigs[0]["return"], "static": sigs[0]["static"]}
            for cls, methods in java_symbols(source).items() for m, sigs in methods.items()}


def normalize_type(decl: str, is_param: bool = True) -> str:
    """Type of a parameter or return declaration, ignoring the parameter name,
    const/final, references and spacing; int* and int[] count as the same."""
    t = re.sub(r"\b(?:const|final|static|inline|extern|volatile|register|public|private|protected)\b", " ", decl or "")
    t = t.replace("&", " ")
    t = re.sub(r"\s*\[\s*\d*\s*\]", " * ", t)
    t = re.sub(r"\*", " * ", t)
    tokens = t.split()
    builtin = {"int", "long", "short", "char", "double", "float", "unsigned", "signed", "bool", "void"}
    if is_param and len(tokens) >= 2 and re.fullmatch(r"[A-Za-z_]\w*", tokens[-1]) \
            and tokens[-1] not in builtin:
        # drop the parameter name; a trailing '*' after it (from name[]) stays
        tokens = tokens[:-1]
    elif is_param and len(tokens) >= 3 and tokens[-1] == "*" and re.fullmatch(r"[A-Za-z_]\w*", tokens[-2]) \
            and tokens[-3] not in ("*",):
        tokens = tokens[:-2] + ["*"]
    return "".join(tokens)


def signature_match(sym: dict, cand: dict) -> bool | None:
    """True/False when both sides declare types; None when they cannot be compared (Python)."""
    if cand.get("params") is None or "params" not in sym:
        return None
    if len(sym["params"]) != len(cand["params"]):
        return False
    same_params = all(normalize_type(a) == normalize_type(b) for a, b in zip(sym["params"], cand["params"]))
    same_return = normalize_type(sym.get("return", ""), False) == normalize_type(cand.get("return", ""), False)
    return same_params and same_return


def draft_adapter(language: str, oracle: dict, artifact_source: str) -> dict:
    """Propose a name binding for each canonical symbol (static analysis only).
    Nothing is approved here: a human must set status to "approved".

    proposed_exact  same name and arity, and same parameter/return types where
                    the language declares them
    proposed_loose  name equal ignoring case/_ with same arity, or exact name
                    with differing types, or a binding to main
    unresolved      no unique candidate
    """
    available = artifact_symbols(language, artifact_source)
    bindings, ranks = [], []
    for sym in oracle["symbols"]:
        canonical = sym["canonical"]
        arity = sym.get("arity", len(sym.get("params", [])) if "params" in sym else None)
        if language == "java":
            target_name = sym["method"]
            candidates = [n for n in available if n.split(".")[1] == target_name]
            norm = [n for n in available if normalize_name(n.split(".")[1]) == normalize_name(target_name)]
        else:
            target_name = canonical if language != "python" else sym["original"]
            candidates = [n for n in available if n == canonical or n == target_name]
            norm = [n for n in available if normalize_name(n) in {normalize_name(canonical), normalize_name(target_name)}]
        same_arity = lambda n: arity is None or available[n]["arity"] in (None, arity)
        exact = [n for n in candidates if same_arity(n)]
        loose = [n for n in norm if same_arity(n)]
        if len(exact) == 1 or len(loose) == 1:
            target = exact[0] if len(exact) == 1 else loose[0]
            types = signature_match(sym, available[target])
            name_exact = len(exact) == 1
            rank = "proposed_exact" if name_exact and types is not False and target_name != "main" else "proposed_loose"
            bindings.append({"canonical": canonical, "target": target, "kind": "alias",
                             "name_match": "exact" if name_exact else "normalized", "types_match": types,
                             "canonical_signature": {k: sym.get(k) for k in ("return", "params", "arity") if k in sym},
                             "target_signature": available[target]})
            ranks.append(rank)
        else:
            bindings.append({"canonical": canonical, "target": None, "kind": "unresolved",
                             "candidates": sorted(set(candidates + norm))})
            ranks.append("unresolved")
    if oracle.get("instance_use"):
        ranks.append("unresolved")
    status = ("unresolved" if "unresolved" in ranks or not bindings else
              "proposed_loose" if "proposed_loose" in ranks else "proposed_exact")
    return {"status": status, "bindings": bindings}


def validate_adapter(adapter: dict, artifact_sha256: str, oracle: dict) -> list[str]:
    """Problems that make an adapter unusable (empty list = usable). Only an
    adapter a human has set to "approved" is usable."""
    problems = []
    if adapter.get("artifact_sha256") != artifact_sha256:
        problems.append("artifact changed since the adapter was written")
    if adapter.get("oracle_test_sha256") != oracle["test_sha256"]:
        problems.append("oracle test changed since the adapter was written")
    if adapter.get("status") != "approved":
        problems.append(f"adapter status is {adapter.get('status')} (needs human approval)")
    elif not adapter.get("reviewer"):
        problems.append("approved adapter has no reviewer")
    canon = {s["canonical"] for s in oracle["symbols"]}
    bound = {b["canonical"] for b in adapter.get("bindings", []) if b.get("target")}
    if canon - bound:
        problems.append("unbound symbols: " + ", ".join(sorted(canon - bound)))
    for b in adapter.get("bindings", []):
        if b.get("kind") != "alias":
            problems.append(f"binding kind {b.get('kind')} is not a pure alias")
        if b.get("target") and not re.fullmatch(r"[A-Za-z_$][\w$]*(?:\.[A-Za-z_$][\w$]*)?", b["target"]):
            problems.append(f"invalid target name {b['target']!r}")
    return problems


# ─────────────────────────────────────────────────────────────────────────────
#  WORKSPACES (file operations only)
# ─────────────────────────────────────────────────────────────────────────────

def _replace_binding_lines(test_text: str, binding_lines: list[str], replacement: str) -> tuple[str, list]:
    """Replace the binding lines with one replacement line (at the first one)."""
    out, changes, inserted = [], [], False
    targets = {b.strip() for b in binding_lines}
    for no, line in enumerate(test_text.splitlines(keepends=True), start=1):
        if line.strip() in targets:
            changes.append({"line": no, "removed": line.rstrip("\r\n")})
            if not inserted:
                out.append(replacement + "\n"); inserted = True
                changes[-1]["inserted"] = replacement
            continue
        out.append(line)
    return "".join(out), changes


def build_workspace(language: str, ws: Path, artifact_path: Path, oracle: dict, adapter: dict,
                    test_text: str, artifact_origin: str = "generated", approvals: dict | None = None) -> dict:
    """Create the test workspace. The artifact and the test are copied; the
    originals are never modified. Returns the workspace manifest."""
    ws.mkdir(parents=True, exist_ok=False)
    artifact = artifact_path.read_bytes()
    targets = {b["canonical"]: b["target"] for b in adapter["bindings"]}
    files, changes = {}, []
    if language == "python":
        files["impl_under_test.py"] = artifact
        lines = ["# Generated adapter: binds the oracle's names to the artifact. No logic."]
        lines += [f"from impl_under_test import {targets[s['canonical']]} as {s['canonical']}" for s in oracle["symbols"]]
        files["adapter_bindings.py"] = ("\n".join(lines) + "\n").encode()
        new_test, changes = _replace_binding_lines(test_text, oracle["binding_lines"],
                                                   "from adapter_bindings import *  # adapter")
        files["test_oracle.py"] = new_test.encode("utf-8")
        steps = ([["{python}", "-m", "unittest", "-v", "test_oracle"]] if oracle["verdict_style"] == "unittest"
                 else [["{python}", "test_oracle.py"]])
    elif language in ("c", "cpp"):
        ext = "c" if language == "c" else "cpp"
        files[f"impl_under_test.{ext}"] = artifact
        lines = ["/* Generated adapter: binds the oracle's names to the artifact. No logic. */",
                 "#define main impl_under_test_main", f'#include "impl_under_test.{ext}"', "#undef main"]
        lines += [f"#define {s['canonical']} {targets[s['canonical']]}" for s in oracle["symbols"]
                  if targets[s["canonical"]] != s["canonical"]]
        files["adapter.h"] = ("\n".join(lines) + "\n").encode()
        new_test, changes = _replace_binding_lines(test_text, oracle["binding_lines"], '#include "adapter.h"  /* adapter */')
        files[f"test_oracle.{ext}"] = new_test.encode("utf-8")
        compiler, std = ("{cc}", "-std=gnu17") if language == "c" else ("{cxx}", "-std=gnu++17")
        steps = [[compiler, std, "-O0", "-o", "{exe:test_oracle}", f"test_oracle.{ext}", "-lm", "-lpthread"],
                 ["{run:test_oracle}"]]
    else:
        pkg = oracle.get("package", "")
        pkg_dir = Path(*pkg.split(".")) if pkg else Path()
        art_text = artifact.decode("utf-8", errors="replace")
        art_name = java_file_name(code_only(art_text, "java"), "ImplUnderTest.java")
        if re.search(r"^\s*package\s+[\w.]+\s*;", art_text, re.M):
            art_new = re.sub(r"^\s*package\s+[\w.]+\s*;", f"package {pkg};" if pkg else "", art_text, count=1, flags=re.M)
            changes.append({"file": art_name, "change": f"package declaration replaced with {pkg!r}"})
        else:
            art_new = (f"package {pkg};\n" if pkg else "") + art_text
            changes.append({"file": art_name, "change": f"package {pkg} prepended"})
        files[(pkg_dir / art_name).as_posix()] = art_new.encode("utf-8")
        art_classes = java_symbols(art_text)
        by_class = defaultdict(list)
        for s in oracle["symbols"]:
            by_class[s["class"]].append(s)
        for cls, syms in by_class.items():
            if all(targets[s["canonical"]] == s["canonical"] for s in syms) and cls in art_classes:
                continue  # the artifact already provides this class and these methods
            # java.util/java.io cover the parameter types used by the oracles (List, Map, ...).
            body = [f"package {pkg};" if pkg else "", "import java.util.*;", "import java.io.*;",
                    "/* Generated adapter facade: delegates only. No logic. */",
                    f"public class {cls} {{"]
            for s in syms:
                t_cls, t_method = targets[s["canonical"]].split(".")
                params = s.get("params", [])
                names = [p.split()[-1].lstrip("*&").split("[")[0] for p in params]
                ret = s.get("return", "void")
                call = f"{t_cls}.{t_method}({', '.join(names)})"
                body.append(f"    public static {ret} {s['method']}({', '.join(params)}) {{ "
                            f"{'' if ret == 'void' else 'return '}{call}; }}")
            body.append("}")
            files[(pkg_dir / f"{cls}.java").as_posix()] = ("\n".join(body) + "\n").encode()
        test_name = Path(oracle["test_path"]).name
        files[(pkg_dir / test_name).as_posix()] = test_text.encode("utf-8")
        test_class = (pkg + "." if pkg else "") + Path(test_name).stem
        sources = sorted(str(Path(p).as_posix()) for p in files)
        steps = [["{javac}", "-encoding", "UTF-8", "-proc:none", "-d", "classes", "-cp", "{junit_jar}", *sources],
                 ["{java}", "-jar", "{junit_jar}", "execute", "--class-path", "classes", "--select-class", test_class,
                  "--reports-dir", "reports", "--disable-banner", "--details=summary"]]
    for rel_path, data in files.items():
        dest = ws / rel_path
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(data)
    manifest = {
        "language": language, "artifact_origin": artifact_origin,
        "artifact_sha256": sha256_bytes(artifact), "oracle_test_path": oracle["test_path"],
        "oracle_test_sha256": oracle["test_sha256"], "verdict_style": oracle["verdict_style"],
        "bindings": adapter["bindings"], "changes": changes, "steps": steps, "approvals": approvals or {},
        "files": {p: sha256_bytes(d) for p, d in sorted(files.items())},
    }
    (ws / "workspace.json").write_text(json.dumps(manifest, indent=1) + "\n", encoding="utf-8")
    return manifest


# ─────────────────────────────────────────────────────────────────────────────
#  VERDICTS
# ─────────────────────────────────────────────────────────────────────────────

_ASSERT_FAIL = re.compile(r"Assertion `.*' failed|Assertion failed:|assertion .* failed", re.I)


def verdict(language: str, verdict_style: str, steps: list[dict], workspace: Path | None = None) -> tuple[str, str]:
    """Classify executed steps into (category, reason). Each step is
    {returncode, output, timeout}. The last step is the test run."""
    for step in steps[:-1]:
        if step.get("timeout"):
            return TIMEOUT, "build timed out"
        if step["returncode"] != 0:
            return ERROR, "build_failed"
    if not steps:
        return ERROR, "sandbox produced no step output"
    run = steps[-1]
    if run.get("timeout"):
        return TIMEOUT, "test run exceeded the time limit"
    out = run["output"]
    if verdict_style == "unittest":
        ran = re.search(r"^Ran (\d+) tests?", out, re.M)
        if not ran:
            return ERROR, "test module did not load (import or top-level error)"
        if int(ran.group(1)) == 0:
            return ERROR, "no tests ran"
        if re.search(r"^OK\b", out, re.M) and run["returncode"] == 0:
            return PASSED, f"{ran.group(1)} tests"
        res = re.search(r"^FAILED \(([^)]*)\)", out, re.M)
        counts = dict(re.findall(r"(\w+)=(\d+)", res.group(1))) if res else {}
        if int(counts.get("failures", 0)) > 0:
            return FAILED, f"failures={counts.get('failures')} errors={counts.get('errors', 0)} of {ran.group(1)}"
        return ERROR, f"errors={counts.get('errors', '?')} of {ran.group(1)} (exceptions, no assertion failures)"
    if verdict_style == "exit_code":  # Python script with bare asserts
        if run["returncode"] == 0:
            return PASSED, "exit 0"
        return (FAILED, "AssertionError") if "AssertionError" in out else (ERROR, f"exit {run['returncode']}")
    if verdict_style == "exit_code_and_fail_lines":
        fails = len(re.findall(r"\bFAIL\b", out))
        if fails:
            return FAILED, f"{fails} FAIL line(s)"
        if _ASSERT_FAIL.search(out):
            return FAILED, "assertion failed"
        if run["returncode"] == 0:
            return PASSED, "exit 0, no FAIL lines"
        return ERROR, f"crash or abnormal exit {run['returncode']}"
    if verdict_style == "junit_xml":
        reports = sorted((workspace / "reports").glob("*.xml")) if workspace else []
        if not reports:
            return ERROR, "no JUnit report"
        total = failed = errored = 0
        for report in reports:
            for case in ET.parse(report).getroot().iter("testcase"):
                total += 1
                for child in case:
                    kind = child.get("type", "")
                    if child.tag == "failure" and ("AssertionFailedError" in kind or "AssertionError" in kind):
                        failed += 1
                    elif child.tag in ("failure", "error"):
                        errored += 1
        if total == 0:
            return ERROR, "no tests ran"
        if failed:
            return FAILED, f"failures={failed} errors={errored} of {total}"
        if errored:
            return ERROR, f"errors={errored} of {total} (exceptions, no assertion failures)"
        return PASSED, f"{total} tests"
    return ERROR, f"unknown verdict style {verdict_style}"


# ─────────────────────────────────────────────────────────────────────────────
#  EXECUTORS
# ─────────────────────────────────────────────────────────────────────────────

class NoSandbox:
    """Default: no isolated Linux environment is configured, so nothing runs."""
    name = "none"

    def available(self) -> tuple[bool, str]:
        return False, "sandbox_unavailable"


class TrustedFixtureExecutor:
    """Runs workspaces built from the framework's own test fixtures on the host.
    Refuses any workspace whose code files are not on the fixture allowlist,
    so it can never run a generated artifact."""
    name = "trusted_fixture"

    def __init__(self, allowlist_dir: Path, tools: dict, timeout: float = 20, build_timeout: float = 120):
        self.allowed = {sha256_bytes(p.read_bytes()) for p in Path(allowlist_dir).rglob("*") if p.is_file()}
        self.tools, self.timeout, self.build_timeout = tools, timeout, build_timeout

    def available(self) -> tuple[bool, str]:
        return True, ""

    def run(self, ws: Path) -> list[dict]:
        manifest = json.loads((ws / "workspace.json").read_text(encoding="utf-8"))
        if manifest["artifact_origin"] != "fixture" or manifest["artifact_sha256"] not in self.allowed \
                or manifest["oracle_test_sha256"] not in self.allowed:
            raise PermissionError("TrustedFixtureExecutor only runs allowlisted fixture code")
        exe = ".exe" if os.name == "nt" else ""
        results = []
        for index, step in enumerate(manifest["steps"]):
            is_test_run = index == len(manifest["steps"]) - 1
            argv = []
            for arg in step:
                if arg.startswith("{exe:"):
                    argv.append(arg[5:-1] + exe)
                elif arg.startswith("{run:"):
                    argv.append(str(ws / (arg[5:-1] + exe)))
                elif arg.startswith("{") and arg.endswith("}"):
                    argv.extend(self.tools[arg[1:-1]])
                else:
                    argv.append(arg)
            try:
                proc = subprocess.run(argv, cwd=ws, capture_output=True,
                                      timeout=self.timeout if is_test_run else self.build_timeout,
                                      stdin=subprocess.DEVNULL, env=_fixture_env())
                out = (proc.stdout + proc.stderr).decode("utf-8", errors="replace").replace("\r\n", "\n")
                results.append({"returncode": proc.returncode, "output": out, "timeout": False})
            except subprocess.TimeoutExpired:
                results.append({"returncode": None, "output": "", "timeout": True})
                break
            if proc.returncode != 0 and len(results) < len(manifest["steps"]):
                break
        return pad_skipped_steps(results, len(manifest["steps"]))


SANDBOX_DIR = EVAL_DIR / "sandbox"
WSL_TOOLS = {"python": "python3", "cc": "gcc", "cxx": "g++", "javac": "javac", "java": "java -Xmx512m",
             "junit_jar": "/opt/junit/junit-platform-console-standalone-1.11.4.jar"}
# Per-language limits: (build step seconds, test run seconds, address space MB, processes, file size MB).
# The JVM needs a large address space and many threads (threads count as processes).
WSL_LIMITS = {"python": (0, 60, 1024, 64, 50), "c": (120, 30, 2048, 64, 50),
              "cpp": (180, 30, 2048, 64, 50), "java": (180, 90, 6144, 512, 50)}


class WslBwrapExecutor:
    """Runs workspaces in the eval-sandbox WSL distro, each run in a fresh
    bubblewrap sandbox (see sandbox/run_isolated.sh and SANDBOX_PROPOSAL.md).

    Refuses to run anything unless
      - the distro hardening and sandbox security self-tests pass at start-up,
      - every workspace file still has the hash recorded in its manifest, and
      - the workspace is an allowlisted fixture, or its manifest records a human
        approval of both the oracle and the adapter.
    The sandbox protects the machine. It cannot stop code under test from
    faking its own test output, because the two share a process."""
    name = "wsl_bwrap"

    def __init__(self, distro: str = "eval-sandbox", fixture_allowlist_dir: Path | None = None,
                 wsl: str = "wsl.exe", self_test: bool = True, limits: dict | None = None):
        self.distro, self.wsl = distro, wsl
        self.limits = {**WSL_LIMITS, **(limits or {})}
        self.allowed = ({sha256_bytes(p.read_bytes()) for p in Path(fixture_allowlist_dir).rglob("*") if p.is_file()}
                        if fixture_allowlist_dir else set())
        self.self_test_report, self.ok, self.reason = [], False, "self-test not run"
        if self_test:
            self.run_self_test()

    def _wsl(self, args: list, stdin: bytes | None, timeout: float) -> subprocess.CompletedProcess:
        return subprocess.run([self.wsl, "-d", self.distro, *args], input=stdin, capture_output=True, timeout=timeout)

    def run_self_test(self) -> None:
        report = []
        try:
            for script in ("check_hardening.sh", "check_bwrap.sh"):
                proc = self._wsl(["--", "bash", "-s"], (SANDBOX_DIR / script).read_bytes(), 900)
                text = proc.stdout.decode("utf-8", errors="replace").replace("\x00", "").replace("\r", "")
                report += [l for l in text.splitlines() if l.startswith(("CHECK ", "INFO "))]
        except (OSError, subprocess.TimeoutExpired) as exc:
            self.ok, self.reason, self.self_test_report = False, f"sandbox self-test could not run: {exc}", report
            return
        checks = [l for l in report if l.startswith("CHECK ")]
        failed = [l for l in checks if " FAIL " in l]
        self.self_test_report = report
        if not checks:
            self.ok, self.reason = False, "sandbox self-test produced no results"
        elif failed:
            self.ok, self.reason = False, "sandbox self-test failed: " + "; ".join(failed)
        else:
            self.ok, self.reason = True, ""

    def available(self) -> tuple[bool, str]:
        return (True, "") if self.ok else (False, f"sandbox_unavailable ({self.reason})")

    def authorize(self, ws: Path, manifest: dict) -> None:
        """Raise PermissionError unless this workspace may run."""
        if not self.ok:
            raise PermissionError(f"sandbox not verified: {self.reason}")
        for rel_path, digest in manifest["files"].items():
            if sha256_bytes((ws / rel_path).read_bytes()) != digest:
                raise PermissionError(f"workspace file changed after it was built: {rel_path}")
        if manifest["artifact_origin"] == "fixture":
            if manifest["artifact_sha256"] in self.allowed and manifest["oracle_test_sha256"] in self.allowed:
                return
            raise PermissionError("fixture workspace is not on the fixture allowlist")
        approvals = manifest.get("approvals") or {}
        if approvals.get("oracle") != "approved" or approvals.get("adapter") != "approved" \
                or not approvals.get("adapter_reviewer"):
            raise PermissionError("generated code runs only with a human-approved oracle and adapter")

    def driver_script(self, manifest: dict, nonce: str) -> str:
        build_s, run_s, *_ = self.limits[manifest["language"]]
        lines = ["#!/bin/bash", "cp -a /src/. /ws/ && rm -f /ws/.eval_driver.sh || exit 125", "cd /ws",
                 f'step() {{ local i=$1 t=$2; shift 2; echo "@@{nonce} STEP $i BEGIN@@"; '
                 f'timeout -k 2 "$t" "$@"; local rc=$?; echo; echo "@@{nonce} STEP $i END rc=$rc@@"; return $rc; }}']
        steps = manifest["steps"]
        for index, step in enumerate(steps):
            argv = []
            for arg in step:
                if arg.startswith("{exe:"):
                    argv.append(arg[5:-1])
                elif arg.startswith("{run:"):
                    argv.append("./" + arg[5:-1])
                elif arg.startswith("{") and arg.endswith("}"):
                    argv.extend(WSL_TOOLS[arg[1:-1]].split())
                else:
                    argv.append(arg)
            limit = run_s if index == len(steps) - 1 else build_s
            quoted = " ".join("'" + a.replace("'", "'\\''") + "'" for a in argv)
            lines.append(f"step {index} {limit} {quoted}" + (" || exit 0" if index < len(steps) - 1 else ""))
        lines.append(f'for f in reports/*.xml; do [ -f "$f" ] && {{ echo "@@{nonce} FILE $f@@"; '
                     f'base64 -w0 "$f"; echo; echo "@@{nonce} ENDFILE@@"; }}; done; exit 0')
        return "\n".join(lines) + "\n"

    def run(self, ws: Path) -> list[dict]:
        import io
        import secrets
        import tarfile
        manifest = json.loads((ws / "workspace.json").read_text(encoding="utf-8"))
        self.authorize(ws, manifest)
        nonce = secrets.token_hex(8)
        build_s, run_s, mem, nproc, fsize = self.limits[manifest["language"]]
        total = build_s * (len(manifest["steps"]) - 1) + run_s + 30
        buf = io.BytesIO()
        with tarfile.open(fileobj=buf, mode="w") as tar:
            for rel_path in manifest["files"]:
                tar.add(ws / rel_path, arcname=rel_path)
            data = self.driver_script(manifest, nonce).encode()
            info = tarfile.TarInfo(".eval_driver.sh"); info.size = len(data); info.mode = 0o644
            tar.addfile(info, io.BytesIO(data))
        try:
            proc = self._wsl(["--", "/opt/eval-sandbox/receive_and_run.sh", str(total), str(mem), str(nproc), str(fsize)],
                             buf.getvalue(), total + 60)
        except subprocess.TimeoutExpired:
            return [{"returncode": None, "output": "", "timeout": True}]
        out = proc.stdout.decode("utf-8", errors="replace")
        (ws / "sandbox_output.txt").write_text(out, encoding="utf-8")
        return pad_skipped_steps(self.parse_output(out, nonce, len(manifest["steps"]), proc.returncode, ws),
                                 len(manifest["steps"]))

    @staticmethod
    def parse_output(out: str, nonce: str, n_steps: int, returncode: int, ws: Path | None) -> list[dict]:
        results = []
        for i in range(n_steps):
            begin = out.find(f"@@{nonce} STEP {i} BEGIN@@")
            if begin < 0:
                break
            end = re.search(rf"@@{nonce} STEP {i} END rc=(\d+)@@", out[begin:])
            body_start = begin + len(f"@@{nonce} STEP {i} BEGIN@@") + 1
            if not end:  # killed by the overall limit
                results.append({"returncode": None, "output": out[body_start:], "timeout": True})
                break
            rc = int(end.group(1))
            results.append({"returncode": rc, "output": out[body_start:begin + end.start()],
                            "timeout": rc in (124, 137)})
        if not results and returncode in (124, 137):
            results.append({"returncode": None, "output": out, "timeout": True})
        if ws is not None:
            for m in re.finditer(rf"@@{nonce} FILE (reports/[\w.\-]+\.xml)@@\n(.*?)\n@@{nonce} ENDFILE@@", out, re.S):
                import base64
                dest = ws / m.group(1)
                dest.parent.mkdir(parents=True, exist_ok=True)
                dest.write_bytes(base64.b64decode(m.group(2)))
        return results


def pad_skipped_steps(results: list[dict], n_steps: int) -> list[dict]:
    """After a failed build the remaining steps are not run. Mark them as
    skipped so verdict() sees the failed build step, not a short list whose
    last entry looks like the test run."""
    if results and len(results) < n_steps and not results[-1].get("timeout"):
        results = results + [{"returncode": None, "output": "", "timeout": False, "skipped": True}
                             for _ in range(n_steps - len(results))]
    return results


def _fixture_env() -> dict:
    keep = ("SYSTEMROOT", "SystemRoot", "WINDIR", "PATH", "Path", "TEMP", "TMP", "PATHEXT", "COMSPEC",
            "LOCALAPPDATA", "USERPROFILE", "HOME")
    env = {k: v for k, v in os.environ.items() if k in keep}
    cache = str(EVAL_DIR / ".toolchain" / "zig-cache")
    env.update({"ZIG_GLOBAL_CACHE_DIR": cache, "ZIG_LOCAL_CACHE_DIR": cache, "PYTHONDONTWRITEBYTECODE": "1"})
    return env


# ─────────────────────────────────────────────────────────────────────────────
#  PLAN
# ─────────────────────────────────────────────────────────────────────────────

def plan(gate1_dir: Path, out_dir: Path, catalog: dict, review: dict, executor, workspaces: bool,
         adapter_review: dict | None = None) -> list[dict]:
    adapter_review = adapter_review or {}
    inventory = {r["combo_index"]: r for r in json.loads((gate1_dir / "inventory.json").read_text(encoding="utf-8"))["rows"]}
    gate1 = json.loads((gate1_dir / "gate1_results.json").read_text(encoding="utf-8"))
    project_root = EVAL_DIR.parent
    ok, sandbox_reason = executor.available()
    rows = []
    (out_dir / "adapters").mkdir(parents=True, exist_ok=True)
    for g in gate1:
        inv = inventory[g["combo_index"]]
        row = {f: "" for f in PLAN_FIELDS}
        row.update({k: g[k] for k in ("combo_index", "task_id", "language", "model_label", "gate1_status")})
        row["functional_status"] = NOT_TESTABLE
        oracle_status, oracle = oracle_for(catalog, review, g["task_id"], g["language"])
        row["oracle_status"] = oracle_status
        if oracle:
            row["oracle_original_model"], row["oracle_test_path"] = oracle["original_model"], oracle["test_path"]
        if g["gate1_status"] != "pass":
            row["reason"] = f"gate1:{g['gate1_status']}"
        elif oracle is None:
            row["reason"] = "no_oracle"
        else:
            artifact_path = project_root / inv["source_path"]
            adapter = draft_adapter(g["language"], oracle, read_text(artifact_path))
            decision = adapter_review.get(str(g["combo_index"]))
            if decision and decision.get("artifact_sha256") == inv["source_sha256"]                     and decision.get("oracle_test_sha256") == oracle["test_sha256"]:
                # A human decision for exactly this artifact and test replaces the draft.
                adapter["status"] = "approved" if decision.get("approved") else "rejected"
                adapter["reviewer"] = decision.get("reviewer", "")
                adapter["review_notes"] = decision.get("notes", "")
                if decision.get("bindings"):
                    adapter["bindings"] = [dict(b, kind="alias") for b in decision["bindings"]]
            adapter.update({"combo_index": g["combo_index"], "artifact_path": inv["source_path"],
                            "artifact_sha256": inv["source_sha256"], "oracle_test_path": oracle["test_path"],
                            "oracle_test_sha256": oracle["test_sha256"], "author": "functional.py draft_adapter (static)"})
            name = f"{g['combo_index']:04d}_Task{g['task_id']}_{g['model_label']}_{g['language']}"
            (out_dir / "adapters" / f"{name}.json").write_text(json.dumps(adapter, indent=1) + "\n", encoding="utf-8")
            row["adapter_status"] = adapter["status"]
            row["adapter_bindings"] = ";".join(f"{b['canonical']}->{b['target']}" for b in adapter["bindings"])
            problems = validate_adapter(adapter, inv["source_sha256"], oracle)
            if oracle_status != "approved":
                row["reason"] = f"oracle_{oracle_status}"
            elif problems:
                row["reason"] = "adapter: " + "; ".join(problems)
            elif not ok:
                row["reason"] = sandbox_reason
            else:
                row["reason"] = "ready"
            ready = row["reason"] == "ready"
            if (workspaces or ready) and adapter["status"] not in ("unresolved", "rejected"):
                ws = out_dir / "workspaces" / name
                approvals = {"oracle": oracle_status, "adapter": adapter["status"],
                             "adapter_reviewer": adapter.get("reviewer", "")}
                build_workspace(g["language"], ws, artifact_path, oracle, adapter,
                                read_text(EVAL_DIR / oracle["test_local_path"]), approvals=approvals)
                row["workspace"] = ws.relative_to(out_dir).as_posix()
                if ready:
                    category, why = verdict(g["language"], oracle["verdict_style"], executor.run(ws), ws)
                    row["functional_status"], row["reason"] = category, why
        rows.append(row)
    return rows


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description="Evaluation Milestone 2: functional correctness.")
    sub = parser.add_subparsers(dest="cmd", required=True)
    sub.add_parser("catalog")
    p = sub.add_parser("plan")
    p.add_argument("--gate1", required=True)
    p.add_argument("--out", required=True)
    p.add_argument("--workspaces", action="store_true", help="Also build test workspaces (copies only).")
    p.add_argument("--execute", action="store_true",
                   help="Run approved combinations in the WSL sandbox (after its security self-test passes).")
    args = parser.parse_args(argv)

    if args.cmd == "catalog":
        manifest = json.loads((UPSTREAM_DIR / "manifest.json").read_text(encoding="utf-8"))
        catalog = build_catalog(manifest)
        FUNCTIONAL_DIR.mkdir(exist_ok=True)
        CATALOG_PATH.write_text(json.dumps(catalog, indent=1) + "\n", encoding="utf-8")
        with (FUNCTIONAL_DIR / "oracle_catalog.csv").open("w", encoding="utf-8", newline="") as fh:
            w = csv.writer(fh, lineterminator="\n")
            w.writerow(["task_id", "language", "test_files", "usable_files", "agreement",
                        "reference_model", "reference_test", "bindings_seen", "symbols"])
            for e in catalog["entries"].values():
                ref = next((t for t in e["tests"] if e["reference"] and t["test_path"] == e["reference"]["test_path"]), None)
                w.writerow([e["task_id"], e["language"], e["test_files"], e["usable_files"], e["agreement"],
                            ref["original_model"] if ref else "", ref["test_path"] if ref else "",
                            ";".join(sorted({t["binding"] for t in e["tests"]})),
                            ";".join(s["canonical"] for s in ref["symbols"]) if ref else ""])
        usable = Counter(e["language"] for e in catalog["entries"].values() if e["reference"])
        print(f"Oracle catalog: {dict(usable)} task-language entries with a proposed reference test")
        return 0

    gate1_dir, out_dir = Path(args.gate1).resolve(), Path(args.out).resolve()
    if out_dir.exists() and any(out_dir.iterdir()):
        raise SystemExit(f"Output folder already has files: {out_dir}")
    out_dir.mkdir(parents=True, exist_ok=True)
    catalog = json.loads(CATALOG_PATH.read_text(encoding="utf-8"))
    executor = NoSandbox()
    if args.execute:
        executor = WslBwrapExecutor()
        (out_dir / "sandbox_self_test.txt").write_text("\n".join(executor.self_test_report) + "\n", encoding="utf-8")
        print("sandbox:", "verified" if executor.ok else executor.reason)
    adapter_review = (json.loads(ADAPTER_REVIEW_PATH.read_text(encoding="utf-8"))
                      if ADAPTER_REVIEW_PATH.is_file() else {})
    rows = plan(gate1_dir, out_dir, catalog, load_review(), executor, args.workspaces, adapter_review)
    with (out_dir / "functional_plan.csv").open("w", encoding="utf-8", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=PLAN_FIELDS, lineterminator="\n")
        w.writeheader()
        w.writerows(rows)
    summary = {
        "functional_status": dict(Counter(r["functional_status"] for r in rows)),
        "reasons": dict(Counter(r["reason"].split(":")[0] if r["reason"].startswith("gate1") else r["reason"].split(";")[0]
                                for r in rows).most_common()),
        "oracle_status": dict(Counter(r["oracle_status"] for r in rows)),
        "adapter_status": dict(Counter(r["adapter_status"] for r in rows if r["adapter_status"])),
        "adapter_status_by_language": {l: dict(Counter(r["adapter_status"] for r in rows if r["language"] == l and r["adapter_status"]))
                                       for l in ("python", "java", "c", "cpp")},
    }
    (out_dir / "functional_summary.json").write_text(json.dumps(summary, indent=1) + "\n", encoding="utf-8")
    print(json.dumps(summary, indent=1))
    return 0


if __name__ == "__main__":
    sys.exit(main())
