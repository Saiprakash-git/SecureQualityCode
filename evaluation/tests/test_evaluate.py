import csv
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import evaluate

REAL_CONFIG = Path(__file__).resolve().parents[1] / "eval_config_benchmark_40.json"


def _toolchain():
    try:
        return evaluate.prepare_toolchain(evaluate.load_eval_config(REAL_CONFIG))
    except Exception:
        return None


TC = _toolchain()
needs_toolchain = unittest.skipIf(TC is None, "zig/javac toolchain not installed (see evaluation/README.md)")


class FixtureProject:
    """A tiny project: runner config, dataset, one run folder with a ledger."""

    def __init__(self, root: Path):
        self.root = root
        self.run = root / "output" / "bench" / "run-real"
        (root / "runner").mkdir(parents=True)
        (root / "evaluation").mkdir()
        dataset = root / "dataset"
        dataset.mkdir()
        for tid in (1, 2):
            (dataset / f"promptDescription-{tid:03d}.json").write_text(json.dumps(
                {"tasks": [{"task_number": tid, "prompt_title": f"Task {tid}"}]}), encoding="utf-8")
        plan = {
            "languages": ["python", "c"], "max_attempts": 3,
            "dataset_dirs": ["../dataset"],
            "models": [{"label": "m1", "model_id": "x/m1"}, {"label": "m2", "model_id": "x/m2"}],
            "task_selection": {"mode": "orig_id_list", "orig_task_ids": [1, 2]},
        }
        (root / "runner" / "plan.json").write_text(json.dumps(plan), encoding="utf-8")
        self.config = root / "evaluation" / "cfg.json"
        self.config.write_text(json.dumps({
            "project_root": "..", "plan_config": "../runner/plan.json", "runner_dir": "../runner",
            "runs": ["../output/bench/run-real"],
            "toolchain_dir": str(REAL_CONFIG.parent / ".toolchain"),
        }), encoding="utf-8")
        self.run.mkdir(parents=True)
        self.rows = []

    def add(self, task, lang, model, attempt, *, success=True, finish="stop", source=None,
            billed=True, error=None, write_source=True):
        stem = f"Task{task}_{model}_{lang}"
        task_dir = self.run / f"Task_{task:03d}" / model
        task_dir.mkdir(parents=True, exist_ok=True)
        raw = task_dir / f"{stem}_attempt_{attempt:02d}_response.json"
        raw.write_text("{}", encoding="utf-8")
        if success and write_source:
            (task_dir / f"{stem}.{evaluate.LANG_EXT[lang]}").write_text(source or "", encoding="utf-8")
        self.rows.append({
            "task_id": task, "language": lang, "model_label": model, "requested_model_id": f"x/{model}",
            "attempt": attempt, "finish_reason": finish, "response_status": "success" if success else "error",
            "validation_status": "raw_source" if success else "empty_response",
            "billing": "billed" if billed else "not_billed", "cost_usd": 0.001 if billed else None,
            "error": error, "output_tokens": 10, "raw_response_path": "C:\\elsewhere\\" + str(raw.relative_to(self.run.parent)),
        })

    def write_ledger(self, trailing=""):
        text = "".join(json.dumps(r) + "\n" for r in self.rows) + trailing
        (self.run / "cost_ledger.jsonl").write_text(text, encoding="utf-8")

    def inventory(self, task_filter=None):
        cfg = evaluate.load_eval_config(self.config)
        plan = evaluate.load_plan(cfg)
        return cfg, plan, evaluate.build_inventory(cfg, plan, task_filter)


class InventoryTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.p = FixtureProject(Path(self.tmp.name))

    def tearDown(self):
        self.tmp.cleanup()

    def test_statuses_cover_every_planned_combination(self):
        p = self.p
        p.add(1, "python", "m1", 1, source="x = 1\n")
        p.add(1, "python", "m2", 1, finish="length", source="def f(:\n")
        for attempt in (1, 2, 3):
            p.add(1, "c", "m1", attempt, success=False, error="Source validation failed: empty_response")
        p.add(1, "c", "m2", 1, success=False, billed=False, error="OpenRouter HTTP 429")
        p.add(2, "python", "m1", 1, source="y = 2\n", write_source=False)
        p.write_ledger()
        (p.run / "Task_002" / "m2").mkdir(parents=True)
        (p.run / "Task_002" / "m2" / "Task2_m2_c.c").write_text("int x;\n", encoding="utf-8")
        _, _, inv = p.inventory()
        status = {(r["task_id"], r["language"], r["model_label"]): r["status"] for r in inv["rows"]}
        self.assertEqual(len(inv["rows"]), 8)
        self.assertEqual(status[(1, "python", "m1")], evaluate.GENERATED)
        self.assertEqual(status[(1, "python", "m2")], evaluate.GENERATED_TRUNCATED)
        self.assertEqual(status[(1, "c", "m1")], evaluate.FAILED)
        self.assertEqual(status[(1, "c", "m2")], evaluate.INCOMPLETE)
        self.assertEqual(status[(2, "python", "m1")], evaluate.SOURCE_MISSING)
        self.assertEqual(status[(2, "c", "m2")], evaluate.NOT_ATTEMPTED)
        self.assertEqual(inv["meta"]["orphan_sources"], ["output/bench/run-real/Task_002/m2/Task2_m2_c.c"])
        self.assertEqual([r["combo_index"] for r in inv["rows"]], list(range(1, 9)))
        gen = next(r for r in inv["rows"] if r["status"] == evaluate.GENERATED)
        self.assertEqual(gen["source_path"], "output/bench/run-real/Task_001/m1/Task1_m1_python.py")
        self.assertEqual(len(gen["source_sha256"]), 64)

    def test_partial_last_ledger_line_is_skipped_but_corrupt_middle_line_fails(self):
        p = self.p
        p.add(1, "python", "m1", 1, source="x = 1\n")
        p.write_ledger(trailing='{"task_id": 1, "langu')
        _, _, inv = p.inventory()
        self.assertEqual(inv["meta"]["ledger_partial_lines"], 1)
        (p.run / "cost_ledger.jsonl").write_text("{bad\n" + json.dumps(p.rows[0]) + "\n", encoding="utf-8")
        with self.assertRaises(ValueError):
            p.inventory()

    def test_task_filter_and_lock_flag(self):
        p = self.p
        p.write_ledger()
        (p.run.parent / ".runner.lock").write_text("{}", encoding="utf-8")
        _, _, inv = p.inventory({2})
        self.assertEqual({r["task_id"] for r in inv["rows"]}, {2})
        self.assertEqual(inv["meta"]["planned_combinations"], 8)
        self.assertTrue(inv["meta"]["generation_active"])

    def test_results_folder_must_be_outside_runs_and_empty(self):
        p = self.p
        p.write_ledger()
        with self.assertRaises(SystemExit):
            evaluate.main(["--config", str(p.config), "--out", str(p.run / "eval"), "--inventory-only"])
        out = p.root / "results" / "r1"
        out.mkdir(parents=True)
        (out / "x").write_text("x", encoding="utf-8")
        with self.assertRaises(SystemExit):
            evaluate.main(["--config", str(p.config), "--out", str(out), "--inventory-only"])


class IntegrityAndPythonTests(unittest.TestCase):
    def test_integrity_failures(self):
        self.assertIn("empty_source", evaluate.check_integrity("  \n", "c", False)["failures"])
        self.assertIn("truncated_finish_length", evaluate.check_integrity("int x;", "c", True)["failures"])
        self.assertIn("markdown_fence_residue", evaluate.check_integrity("```c\nint x;\n```", "c", False)["failures"])
        self.assertEqual(evaluate.check_integrity("int main(void){return 0;}", "c", False)["failures"], [])

    def test_code_only_ignores_braces_in_strings_and_comments(self):
        src = 'int f(){ char c = \'{\'; const char* s = "}}"; /* { */ // {\n return 0; }'
        self.assertTrue(evaluate.delimiters_balanced(evaluate.code_only(src, "c")))
        java = 'class A { String t = """\n { \n"""; }'
        self.assertTrue(evaluate.delimiters_balanced(evaluate.code_only(java, "java")))
        cpp = 'auto s = R"x( ) } )x"; int g(){return 1;}'
        self.assertTrue(evaluate.delimiters_balanced(evaluate.code_only(cpp, "cpp")))
        self.assertFalse(evaluate.delimiters_balanced(evaluate.code_only("void f() {", "c")))

    def test_python_syntax_and_imports(self):
        ok = evaluate.check_python("import os, requests\nfrom flask import Flask\nfrom . import x\n", "a.py")
        self.assertEqual(ok["syntax_status"], "ok")
        self.assertEqual(ok["third_party_imports"], ["flask", "requests"])
        bad = evaluate.check_python("def f(:\n  pass\n", "b.py")
        self.assertEqual(bad["syntax_status"], "error")
        self.assertIn("line 1", bad["syntax_message"])

    def test_python_check_never_executes_code(self):
        with tempfile.TemporaryDirectory() as tmp:
            marker = Path(tmp) / "ran.txt"
            src = f"open({str(marker)!r}, 'w').write('x')\nimport sys\nsys.exit(3)\n"
            self.assertEqual(evaluate.check_python(src, "c.py")["syntax_status"], "ok")
            self.assertFalse(marker.exists())


@needs_toolchain
class CompilerTests(unittest.TestCase):
    def java(self, source, name="Task1_m_java.java"):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / name
            path.write_text(source, encoding="utf-8")
            syntax = evaluate.java_parse_batch([path], TC)[str(path.resolve())]
        code = evaluate.code_only(source, "java")
        return syntax, evaluate.check_java(source, code, name, TC)

    def test_java_public_class_is_compiled_under_its_own_name(self):
        syntax, res = self.java("// public class Fake {}\npublic class Solution { static int f(){return 1;} }\n")
        self.assertEqual(syntax["syntax_status"], "ok")
        self.assertEqual(res["build_status"], "ok")
        self.assertEqual(res["java_file_name"], "Solution.java")

    def test_java_syntax_error_semantic_error_and_missing_package(self):
        syntax, _ = self.java("Output: prose\npublic class A {}\n")
        self.assertEqual(syntax["syntax_status"], "error")
        syntax, res = self.java("public class B { int f(){ return undefinedName; } }\n")
        self.assertEqual((syntax["syntax_status"], res["build_status"]), ("ok", "error"))
        _, res = self.java("import org.json.JSONObject;\npublic class C { JSONObject o; }\n")
        self.assertEqual(res["build_status"], "blocked_dependency")
        self.assertEqual(res["missing_dependencies"], ["org.json"])

    def test_c_and_cpp_compile_for_linux_target(self):
        posix = "#include <unistd.h>\n#include <pthread.h>\nint f(void){ return (int)getpid(); }\n"
        self.assertEqual(evaluate.check_c_family(posix, "c", "a.c", TC)["build_status"], "ok")
        cpp = "#include <vector>\n#include <thread>\nint g(){ std::vector<int> v{1,2}; return (int)v.size(); }\n"
        self.assertEqual(evaluate.check_c_family(cpp, "cpp", "b.cpp", TC)["build_status"], "ok")

    def test_c_errors_and_missing_header(self):
        res = evaluate.check_c_family("int f(void){ return undefined_name; }\n", "c", "e.c", TC)
        self.assertEqual(res["build_status"], "error")
        self.assertGreaterEqual(res["build_error_count"], 1)
        res = evaluate.check_c_family("#include <sqlite3.h>\nint f(void){return 0;}\n", "c", "s.c", TC)
        self.assertEqual(res["build_status"], "blocked_dependency")
        self.assertEqual(res["missing_dependencies"], ["sqlite3.h"])
        self.assertNotIn("\r", res["first_error"])

    def test_gate1_status_precedence(self):
        integrity = evaluate.check_integrity("int f(void){return 0;}", "c", True)
        self.assertEqual(evaluate.gate1_status(integrity, {"build_status": "ok"}), "fail_integrity")
        clean = evaluate.check_integrity("int f(void){return 0;}", "c", False)
        self.assertEqual(evaluate.gate1_status(clean, {"build_status": "blocked_dependency"}), "blocked_dependency")
        self.assertEqual(evaluate.gate1_status(clean, {"syntax_status": "error", "build_status": "error"}), "fail_syntax")


@needs_toolchain
class EndToEndTests(unittest.TestCase):
    def test_full_run_is_reproducible(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = FixtureProject(Path(tmp))
            p.add(1, "python", "m1", 1, source="x = 1\n")
            p.add(1, "c", "m1", 1, source="int f(void){ return 0; }\n")
            p.add(1, "c", "m2", 1, source="int f(void){ return nope; }\n")
            p.add(2, "c", "m1", 1, source="#include <openssl/evp.h>\n")
            p.write_ledger()
            outs = []
            for name in ("a", "b"):
                out = p.root / "results" / name
                self.assertEqual(evaluate.main(["--config", str(p.config), "--out", str(out), "--workers", "2"]), 0)
                outs.append(out)
            for f in ("inventory.csv", "gate1_results.csv", "gate1_results.json", "summary.json"):
                self.assertEqual((outs[0] / f).read_bytes(), (outs[1] / f).read_bytes(), f)
            with (outs[0] / "gate1_results.csv").open(encoding="utf-8", newline="") as fh:
                rows = list(csv.DictReader(fh))
            self.assertEqual(len(rows), 8)
            got = {(r["task_id"], r["language"], r["model_label"]): r["gate1_status"] for r in rows}
            self.assertEqual(got[("1", "python", "m1")], "pass")
            self.assertEqual(got[("1", "c", "m1")], "pass")
            self.assertEqual(got[("1", "c", "m2")], "fail_compile")
            self.assertEqual(got[("2", "c", "m1")], "blocked_dependency")
            self.assertEqual(got[("2", "python", "m2")], "no_artifact")


class StatsTests(unittest.TestCase):
    def test_wilson(self):
        self.assertEqual(evaluate.wilson(0, 0), [None, None])
        self.assertEqual(evaluate.wilson(0, 10), [0.0, 0.2775])
        self.assertEqual(evaluate.wilson(10, 10), [0.7225, 1.0])
        lo, hi = evaluate.wilson(148, 159)
        self.assertLess(lo, 148 / 159)
        self.assertGreater(hi, 148 / 159)

    def test_mcnemar_exact(self):
        self.assertEqual(evaluate.mcnemar_exact(0, 0), 1.0)
        self.assertEqual(evaluate.mcnemar_exact(5, 0), 0.0625)
        self.assertEqual(evaluate.mcnemar_exact(1, 5), evaluate.mcnemar_exact(5, 1))
        self.assertEqual(evaluate.mcnemar_exact(5, 1), 0.21875)


if __name__ == "__main__":
    unittest.main()
