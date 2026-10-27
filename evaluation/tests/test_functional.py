import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import evaluate
import functional as F

FIX = Path(__file__).resolve().parent / "fixtures" / "functional"
REAL_CONFIG = Path(__file__).resolve().parents[1] / "eval_config_benchmark_40.json"


def _toolchain():
    try:
        return evaluate.prepare_toolchain(evaluate.load_eval_config(REAL_CONFIG))
    except Exception:
        return None


TC = _toolchain()
needs_toolchain = unittest.skipIf(TC is None, "zig/javac toolchain not installed")


def oracle(language, test_name, orig_name, model="claude"):
    """Analyse a fixture test exactly as the catalog analyses upstream tests."""
    test_path = FIX / test_name
    info = F.analyze_test(language, test_path.read_text(encoding="utf-8"),
                          (FIX / orig_name).read_text(encoding="utf-8"), model)
    info.update({"original_model": model, "test_path": test_name,
                 "test_sha256": F.sha256_bytes(test_path.read_bytes())})
    return info


def adapter_for(language, orc, impl_name, **override):
    impl = FIX / impl_name
    adapter = F.draft_adapter(language, orc, impl.read_text(encoding="utf-8"))
    adapter.update({"artifact_sha256": F.sha256_bytes(impl.read_bytes()),
                    "oracle_test_sha256": orc["test_sha256"]}, **override)
    return adapter


class CatalogAnalysisTests(unittest.TestCase):
    def test_python_binding_uses_own_model_line_and_alias(self):
        orc = oracle("python", "oracle_py_unittest.py", "orig_py.py")
        self.assertEqual(orc["binding"], "import_original")
        self.assertEqual(orc["framework"], "unittest")
        self.assertEqual([(s["canonical"], s["original"], s["arity"]) for s in orc["symbols"]],
                         [("add_numbers", "add_two", 2)])
        self.assertEqual(len(orc["binding_lines"]), 2)  # both models' lines are removed later

    def test_c_include_binding_and_symbols(self):
        orc = oracle("c", "oracle_c_assert.c", "orig_c.c")
        self.assertEqual(orc["binding"], "include_original")
        self.assertEqual([s["canonical"] for s in orc["symbols"]], ["add"])
        self.assertEqual(orc["symbols"][0]["params"], ["int a", "int b"])
        self.assertEqual(orc["checks"], 2)

    def test_c_embedded_reference_is_not_usable(self):
        text = "int add(int a, int b) { return a + b; }\nint main(void){ if (add(1,2)!=3) puts(\"FAIL\"); return 0; }\n"
        info = F.analyze_test("c", text, (FIX / "orig_c.c").read_text(encoding="utf-8"), "claude")
        self.assertEqual(info["binding"], "embedded_reference")

    def test_java_static_import_symbols(self):
        orc = oracle("java", "java/Task900_TestCase.java", "java/orig_java.java")
        self.assertEqual(orc["binding"], "class_reference")
        self.assertEqual(orc["package"], "claude.task900")
        sym = orc["symbols"][0]
        self.assertEqual((sym["canonical"], sym["return"], sym["params"], sym["static"]),
                         ("Task900_CLAUDE_x.add", "int", ["int a", "int b"], True))

    def test_case_fingerprint_ignores_binding_lines_and_comments(self):
        a = "# from output.claude.X import f as g\nassert g(1) == 2  # one\n"
        b = "# from output.gemini.Y import h as g\n# different comment 99\nassert g(1) == 2\n"
        self.assertEqual(F.case_fingerprint(a, [a.splitlines()[0]])[0], F.case_fingerprint(b, [b.splitlines()[0]])[0])
        c = "assert g(1) == 3\n"
        self.assertNotEqual(F.case_fingerprint(a, [])[0], F.case_fingerprint(c, [])[0])

    def test_oracle_requires_matching_review(self):
        catalog = {"entries": {"900/c": {"reference": {"test_path": "t.c"},
                                         "tests": [{"test_path": "t.c", "test_sha256": "abc"}]}}}
        self.assertEqual(F.oracle_for(catalog, {}, 900, "c")[0], "unreviewed")
        self.assertEqual(F.oracle_for(catalog, {"900/c": {"approved": True, "test_sha256": "old"}}, 900, "c")[0], "unreviewed")
        self.assertEqual(F.oracle_for(catalog, {"900/c": {"approved": True, "test_sha256": "abc"}}, 900, "c")[0], "approved")
        self.assertEqual(F.oracle_for(catalog, {}, 901, "c")[0], "no_oracle")


class AdapterTests(unittest.TestCase):
    def setUp(self):
        self.py = oracle("python", "oracle_py_unittest.py", "orig_py.py")

    def test_exact_normalized_and_unresolved(self):
        self.assertEqual(adapter_for("python", self.py, "impl_py_correct.py")["status"], "proposed_exact")
        norm = adapter_for("python", self.py, "impl_py_normalized.py")
        self.assertEqual((norm["status"], norm["bindings"][0]["target"]), ("proposed_loose", "addTwo"))
        self.assertEqual(adapter_for("python", self.py, "impl_py_unmatched.py")["status"], "unresolved")

    def test_binding_to_main_is_only_a_loose_proposal(self):
        orc = {"symbols": [{"canonical": "Orig.main", "class": "Orig", "method": "main", "params": ["String[] args"]}]}
        art = "public class App {\n    public static void main(String[] a) {\n    }\n}\n"
        self.assertEqual(F.draft_adapter("java", orc, art)["status"], "proposed_loose")

    def test_exact_name_with_different_types_is_only_loose(self):
        c = oracle("c", "oracle_c_assert.c", "orig_c.c")
        same = F.draft_adapter("c", c, "int add(int x, int y) { return x + y; }\n")
        self.assertEqual((same["status"], same["bindings"][0]["types_match"]), ("proposed_exact", True))
        wider = F.draft_adapter("c", c, "long add(long a, long b) { return a + b; }\n")
        self.assertEqual((wider["status"], wider["bindings"][0]["types_match"]), ("proposed_loose", False))

    def test_nothing_is_usable_without_human_approval(self):
        for impl in ("impl_py_correct.py", "impl_py_normalized.py"):
            a = adapter_for("python", self.py, impl)
            self.assertTrue(any("needs human approval" in p for p in F.validate_adapter(a, a["artifact_sha256"], self.py)))

    def test_validation_rejects_changed_artifact_and_unreviewed_adapter(self):
        a = adapter_for("python", self.py, "impl_py_correct.py", status="approved", reviewer="tester")
        self.assertEqual(F.validate_adapter(a, a["artifact_sha256"], self.py), [])
        self.assertTrue(F.validate_adapter(a, "different", self.py))
        no_reviewer = dict(a, reviewer="")
        self.assertIn("approved adapter has no reviewer", F.validate_adapter(no_reviewer, a["artifact_sha256"], self.py))
        bad = dict(a, bindings=[{"canonical": "add_numbers", "target": "x; import os", "kind": "alias"}])
        self.assertTrue(any("invalid target" in p for p in F.validate_adapter(bad, a["artifact_sha256"], self.py)))

    def test_workspace_changes_only_binding_lines_and_never_touches_originals(self):
        before = {p: p.read_bytes() for p in FIX.rglob("*") if p.is_file()}
        with tempfile.TemporaryDirectory() as tmp:
            ws = Path(tmp) / "ws"
            m = F.build_workspace("python", ws, FIX / "impl_py_correct.py", self.py,
                                  adapter_for("python", self.py, "impl_py_correct.py"),
                                  (FIX / "oracle_py_unittest.py").read_text(encoding="utf-8"), "fixture")
            original = (FIX / "oracle_py_unittest.py").read_text(encoding="utf-8").splitlines()
            new = (ws / "test_oracle.py").read_text(encoding="utf-8").splitlines()
            kept = [l for l in original if not l.startswith("# from output.")]
            self.assertEqual([l for l in new if "adapter" not in l], kept)
            self.assertEqual(len(m["changes"]), 2)
            self.assertEqual((ws / "impl_under_test.py").read_bytes(), (FIX / "impl_py_correct.py").read_bytes())
            self.assertIn("from impl_under_test import add_two as add_numbers", (ws / "adapter_bindings.py").read_text())
        self.assertEqual(before, {p: p.read_bytes() for p in FIX.rglob("*") if p.is_file()})


@needs_toolchain
class TrustedExecutionTests(unittest.TestCase):
    """Runs only the framework's own fixtures (allowlisted by hash) on the host."""

    @classmethod
    def setUpClass(cls):
        win = ["-target", "x86_64-windows-gnu"] if sys.platform == "win32" else []
        cls.ex = F.TrustedFixtureExecutor(FIX, {
            "python": [sys.executable], "cc": [TC["zig"], "cc", *win], "cxx": [TC["zig"], "c++", *win]}, timeout=20)
        cls.tmp = tempfile.TemporaryDirectory()
        # zig builds the target's C runtime on first use; do it once outside the per-step timeout.
        warm = Path(cls.tmp.name) / "warm.c"
        warm.write_text("int main(void){return 0;}\n", encoding="utf-8")
        subprocess.run([TC["zig"], "cc", *win, str(warm), "-o", str(Path(cls.tmp.name) / "warm.exe")],
                       capture_output=True, timeout=900, env=F._fixture_env())
        warm.unlink()

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def run_case(self, language, orc, impl, adapter=None, test_name=None):
        adapter = adapter or adapter_for(language, orc, impl)
        ws = Path(self.tmp.name) / f"{language}_{impl}_{len(list(Path(self.tmp.name).iterdir()))}"
        m = F.build_workspace(language, ws, FIX / impl, orc, adapter,
                              (FIX / (test_name or orc["test_path"])).read_text(encoding="utf-8"), "fixture")
        if language in ("c", "cpp") and sys.platform == "win32":
            for step in m["steps"]:  # pthread is not needed by the fixtures and absent for the windows target
                if "-lpthread" in step:
                    step.remove("-lpthread")
            (ws / "workspace.json").write_text(json.dumps(m), encoding="utf-8")
        return F.verdict(language, m["verdict_style"], self.ex.run(ws), ws)

    def test_python_categories(self):
        py = oracle("python", "oracle_py_unittest.py", "orig_py.py")
        self.assertEqual(self.run_case("python", py, "impl_py_correct.py")[0], F.PASSED)
        self.assertEqual(self.run_case("python", py, "impl_py_wrong.py")[0], F.FAILED)
        self.assertEqual(self.run_case("python", py, "impl_py_raises.py")[0], F.ERROR)
        self.assertEqual(self.run_case("python", py, "impl_py_toplevel_error.py")[0], F.ERROR)

    def test_python_timeout(self):
        py = oracle("python", "oracle_py_unittest.py", "orig_py.py")
        ex = F.TrustedFixtureExecutor(FIX, {"python": [sys.executable]}, timeout=3)
        ws = Path(self.tmp.name) / "hang"
        m = F.build_workspace("python", ws, FIX / "impl_py_hang.py", py, adapter_for("python", py, "impl_py_hang.py"),
                              (FIX / "oracle_py_unittest.py").read_text(encoding="utf-8"), "fixture")
        self.assertEqual(F.verdict("python", m["verdict_style"], ex.run(ws), ws)[0], F.TIMEOUT)

    def test_c_assert_and_pass_fail_styles(self):
        orc = oracle("c", "oracle_c_assert.c", "orig_c.c")
        self.assertEqual(self.run_case("c", orc, "impl_c_exact.c")[0], F.PASSED)
        self.assertEqual(self.run_case("c", orc, "impl_c_wrong.c")[0], F.FAILED)
        self.assertEqual(self.run_case("c", orc, "impl_c_crash.c")[0], F.ERROR)
        self.assertEqual(self.run_case("c", orc, "impl_c_build_error.c"), (F.ERROR, "build_failed"))
        pf = oracle("c", "oracle_c_passfail.c", "orig_c.c")
        self.assertEqual(self.run_case("c", pf, "impl_c_exact.c")[0], F.PASSED)
        self.assertEqual(self.run_case("c", pf, "impl_c_wrong.c")[0], F.FAILED)

    def test_c_reviewed_rename_with_artifact_main(self):
        orc = oracle("c", "oracle_c_assert.c", "orig_c.c")
        drafted = adapter_for("c", orc, "impl_c_correct.c")
        self.assertEqual(drafted["status"], "unresolved")  # 'plus' is not matched automatically
        reviewed = dict(drafted, status="approved", reviewer="tester",
                        bindings=[{"canonical": "add", "target": "plus", "kind": "alias"}])
        self.assertEqual(F.validate_adapter(reviewed, drafted["artifact_sha256"], orc), [])
        self.assertEqual(self.run_case("c", orc, "impl_c_correct.c", adapter=reviewed)[0], F.PASSED)

    def test_executor_refuses_non_fixture_code(self):
        py = oracle("python", "oracle_py_unittest.py", "orig_py.py")
        with tempfile.TemporaryDirectory() as tmp:
            art = Path(tmp) / "generated.py"
            art.write_text("def add_two(a, b):\n    return a + b + 0\n", encoding="utf-8")
            ws = Path(tmp) / "ws"
            adapter = F.draft_adapter("python", py, art.read_text(encoding="utf-8"))
            F.build_workspace("python", ws, art, py, adapter,
                              (FIX / "oracle_py_unittest.py").read_text(encoding="utf-8"), "fixture")
            with self.assertRaises(PermissionError):
                self.ex.run(ws)
            ws2 = Path(tmp) / "ws2"
            F.build_workspace("python", ws2, FIX / "impl_py_correct.py", py, adapter,
                              (FIX / "oracle_py_unittest.py").read_text(encoding="utf-8"), "generated")
            with self.assertRaises(PermissionError):
                self.ex.run(ws2)

    def test_java_workspace_facade_compiles(self):
        orc = oracle("java", "java/Task900_TestCase.java", "java/orig_java.java")
        adapter = adapter_for("java", orc, "java/impl_java_solution.java")
        self.assertEqual((adapter["status"], adapter["bindings"][0]["target"]), ("proposed_exact", "Solution.add"))
        with tempfile.TemporaryDirectory() as tmp:
            ws = Path(tmp) / "ws"
            m = F.build_workspace("java", ws, FIX / "java/impl_java_solution.java", orc, adapter,
                                  (FIX / "java/Task900_TestCase.java").read_text(encoding="utf-8"), "fixture")
            pkg = ws / "claude" / "task900"
            self.assertTrue((pkg / "Solution.java").read_text().startswith("package claude.task900;"))
            self.assertIn("return Solution.add(a, b);", (pkg / "Task900_CLAUDE_x.java").read_text())
            self.assertIn("import java.util.*;", (pkg / "Task900_CLAUDE_x.java").read_text())
            self.assertEqual((pkg / "Task900_TestCase.java").read_bytes(),
                             (FIX / "java/Task900_TestCase.java").read_bytes())
            # Without JUnit only the non-test sources can be compiled here (javac does not run code).
            res = subprocess.run([TC["javac"], "-proc:none", "-d", str(ws / "classes"),
                                  str(pkg / "Solution.java"), str(pkg / "Task900_CLAUDE_x.java")],
                                 capture_output=True, timeout=120)
            self.assertEqual(res.returncode, 0, res.stderr.decode(errors="replace"))
            self.assertEqual(m["steps"][1][:3], ["{java}", "-jar", "{junit_jar}"])


class VerdictTests(unittest.TestCase):
    def step(self, rc, out="", timeout=False):
        return {"returncode": rc, "output": out, "timeout": timeout}

    def test_unittest_outputs(self):
        v = lambda out, rc: F.verdict("python", "unittest", [self.step(rc, out)])[0]
        self.assertEqual(v("Ran 2 tests in 0.001s\n\nOK\n", 0), F.PASSED)
        self.assertEqual(v("Ran 2 tests in 0.001s\n\nFAILED (failures=1)\n", 1), F.FAILED)
        self.assertEqual(v("Ran 2 tests in 0.001s\n\nFAILED (errors=2)\n", 1), F.ERROR)
        self.assertEqual(v("ImportError: cannot import name\n", 1), F.ERROR)
        self.assertEqual(v("Ran 0 tests in 0.000s\n\nOK\n", 0), F.ERROR)

    def test_c_outputs(self):
        v = lambda steps: F.verdict("c", "exit_code_and_fail_lines", steps)[0]
        self.assertEqual(v([self.step(1, "error: x"), self.step(0)]), F.ERROR)
        self.assertEqual(v([self.step(0), self.step(None, timeout=True)]), F.TIMEOUT)
        self.assertEqual(v([self.step(0), self.step(134, "t: t.c:5: main: Assertion `add(2,3) == 5' failed.")]), F.FAILED)
        self.assertEqual(v([self.step(0), self.step(0, "a: PASS\nb: FAIL\n")]), F.FAILED)
        self.assertEqual(v([self.step(0), self.step(139, "")]), F.ERROR)
        self.assertEqual(v([self.step(0), self.step(0, "a: PASS\n")]), F.PASSED)

    def test_junit_reports(self):
        with tempfile.TemporaryDirectory() as tmp:
            ws = Path(tmp)
            (ws / "reports").mkdir()
            ok = [self.step(0), self.step(1)]
            shutil.copy(FIX / "junit_report_fail.xml", ws / "reports" / "TEST-junit-jupiter.xml")
            self.assertEqual(F.verdict("java", "junit_xml", ok, ws), (F.FAILED, "failures=1 errors=1 of 3"))
            shutil.copy(FIX / "junit_report_error.xml", ws / "reports" / "TEST-junit-jupiter.xml")
            self.assertEqual(F.verdict("java", "junit_xml", ok, ws)[0], F.ERROR)
            (ws / "reports" / "TEST-junit-jupiter.xml").unlink()
            self.assertEqual(F.verdict("java", "junit_xml", ok, ws)[0], F.ERROR)

    def test_failed_build_is_build_failed_not_a_test_outcome(self):
        steps = F.pad_skipped_steps([self.step(1, "error: conflicting types")], 2)
        self.assertEqual(F.verdict("c", "exit_code_and_fail_lines", steps), (F.ERROR, "build_failed"))
        self.assertEqual(F.verdict("java", "junit_xml", F.pad_skipped_steps([self.step(1)], 2)), (F.ERROR, "build_failed"))
        self.assertEqual(len(F.pad_skipped_steps([self.step(None, timeout=True)], 2)), 1)

    def test_no_sandbox_is_never_available(self):
        self.assertEqual(F.NoSandbox().available(), (False, "sandbox_unavailable"))
        self.assertEqual(F.RESULT_CATEGORIES, ["passed", "failed", "error", "timeout", "not_testable"])


if __name__ == "__main__":
    unittest.main()
