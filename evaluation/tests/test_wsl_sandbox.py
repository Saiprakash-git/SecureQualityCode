"""WslBwrapExecutor tests. Only the framework's own fixtures are executed, and
only after the sandbox security self-test has passed."""
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
sys.path.insert(0, str(Path(__file__).resolve().parent))

import functional as F
from test_functional import FIX, oracle, adapter_for


def _distro_present() -> bool:
    try:
        out = subprocess.run(["wsl.exe", "-l", "-q"], capture_output=True, timeout=60).stdout
        return "eval-sandbox" in out.decode("utf-16-le", errors="ignore")
    except (OSError, subprocess.TimeoutExpired):
        return False


@unittest.skipUnless(_distro_present(), "eval-sandbox WSL distro not installed")
class WslSandboxTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        limits = {"python": (0, 8, 1024, 64, 50)}  # short run limit so the hang fixture times out fast
        cls.ex = F.WslBwrapExecutor(fixture_allowlist_dir=FIX, limits=limits)
        cls.tmp = tempfile.TemporaryDirectory()
        cls.count = 0

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def workspace(self, language, orc, impl, origin="fixture", approvals=None, adapter=None):
        WslSandboxTests.count += 1
        ws = Path(self.tmp.name) / f"ws{self.count}"
        F.build_workspace(language, ws, FIX / impl, orc, adapter or adapter_for(language, orc, impl),
                          (FIX / orc["test_path"]).read_text(encoding="utf-8"), origin, approvals)
        return ws

    def outcome(self, language, orc, impl, **kw):
        ws = self.workspace(language, orc, impl, **kw)
        return F.verdict(language, orc["verdict_style"], self.ex.run(ws), ws)

    def test_self_test_passed(self):
        self.assertTrue(self.ex.ok, self.ex.reason)
        checks = [l for l in self.ex.self_test_report if l.startswith("CHECK ")]
        self.assertGreaterEqual(len(checks), 30)
        self.assertFalse([l for l in checks if " FAIL " in l])

    def test_python_categories_in_sandbox(self):
        py = oracle("python", "oracle_py_unittest.py", "orig_py.py")
        self.assertEqual(self.outcome("python", py, "impl_py_correct.py")[0], F.PASSED)
        self.assertEqual(self.outcome("python", py, "impl_py_wrong.py")[0], F.FAILED)
        self.assertEqual(self.outcome("python", py, "impl_py_raises.py")[0], F.ERROR)
        self.assertEqual(self.outcome("python", py, "impl_py_toplevel_error.py")[0], F.ERROR)
        self.assertEqual(self.outcome("python", py, "impl_py_hang.py")[0], F.TIMEOUT)

    def test_c_categories_in_sandbox(self):
        orc = oracle("c", "oracle_c_assert.c", "orig_c.c")
        self.assertEqual(self.outcome("c", orc, "impl_c_exact.c")[0], F.PASSED)
        self.assertEqual(self.outcome("c", orc, "impl_c_wrong.c")[0], F.FAILED)
        self.assertEqual(self.outcome("c", orc, "impl_c_crash.c")[0], F.ERROR)
        self.assertEqual(self.outcome("c", orc, "impl_c_build_error.c"), (F.ERROR, "build_failed"))
        pf = oracle("c", "oracle_c_passfail.c", "orig_c.c")
        self.assertEqual(self.outcome("c", pf, "impl_c_wrong.c")[0], F.FAILED)
        drafted = adapter_for("c", orc, "impl_c_correct.c")
        reviewed = dict(drafted, status="approved", reviewer="tester",
                        bindings=[{"canonical": "add", "target": "plus", "kind": "alias"}])
        self.assertEqual(self.outcome("c", orc, "impl_c_correct.c", adapter=reviewed)[0], F.PASSED)

    def test_java_junit_in_sandbox(self):
        orc = oracle("java", "java/Task900_TestCase.java", "java/orig_java.java")
        self.assertEqual(self.outcome("java", orc, "java/impl_java_solution.java"), (F.PASSED, "2 tests"))
        category, reason = self.outcome("java", orc, "java/impl_java_wrong.java")
        self.assertEqual((category, reason), (F.FAILED, "failures=2 errors=0 of 2"))

    def test_refuses_generated_code_without_human_approvals(self):
        py = oracle("python", "oracle_py_unittest.py", "orig_py.py")
        for approvals in (None, {"oracle": "approved", "adapter": "proposed_exact"},
                          {"oracle": "unreviewed", "adapter": "approved", "adapter_reviewer": "x"},
                          {"oracle": "approved", "adapter": "approved", "adapter_reviewer": ""}):
            ws = self.workspace("python", py, "impl_py_correct.py", origin="generated", approvals=approvals)
            with self.assertRaises(PermissionError):
                self.ex.run(ws)

    def test_refuses_tampered_workspace_and_unlisted_fixture(self):
        py = oracle("python", "oracle_py_unittest.py", "orig_py.py")
        ws = self.workspace("python", py, "impl_py_correct.py")
        (ws / "impl_under_test.py").write_text("import os\n", encoding="utf-8")
        with self.assertRaises(PermissionError):
            self.ex.run(ws)
        with tempfile.TemporaryDirectory() as tmp:
            art = Path(tmp) / "x.py"
            art.write_text("def add_two(a, b):\n    return a + b + 0\n", encoding="utf-8")
            ws2 = Path(tmp) / "ws"
            F.build_workspace("python", ws2, art, py, adapter_for("python", py, "impl_py_correct.py"),
                              (FIX / "oracle_py_unittest.py").read_text(encoding="utf-8"), "fixture")
            with self.assertRaises(PermissionError):
                self.ex.run(ws2)

    def test_refuses_everything_when_self_test_failed(self):
        ex = F.WslBwrapExecutor(fixture_allowlist_dir=FIX, self_test=False)
        self.assertFalse(ex.available()[0])
        py = oracle("python", "oracle_py_unittest.py", "orig_py.py")
        with self.assertRaises(PermissionError):
            ex.run(self.workspace("python", py, "impl_py_correct.py"))


class DriverParsingTests(unittest.TestCase):
    def test_markers_need_the_nonce_and_timeouts_are_detected(self):
        out = ("@@abc STEP 0 BEGIN@@\nbuild ok\n\n@@abc STEP 0 END rc=0@@\n"
               "@@abc STEP 1 BEGIN@@\nfake: @@zzz STEP 1 END rc=0@@\nrunning\n\n@@abc STEP 1 END rc=124@@\n")
        steps = F.WslBwrapExecutor.parse_output(out, "abc", 2, 0, None)
        self.assertEqual([s["returncode"] for s in steps], [0, 124])
        self.assertTrue(steps[1]["timeout"])
        killed = F.WslBwrapExecutor.parse_output("@@abc STEP 0 BEGIN@@\nloop", "abc", 1, 124, None)
        self.assertTrue(killed[0]["timeout"])

    def test_driver_quotes_arguments(self):
        ex = F.WslBwrapExecutor(self_test=False)
        script = ex.driver_script({"language": "c", "steps": [["{cc}", "-o", "{exe:t}", "it's.c"], ["{run:t}"]]}, "n1")
        self.assertIn("step 0 120 'gcc' '-o' 't' 'it'\\''s.c' || exit 0", script)
        self.assertIn("step 1 30 './t'", script)


if __name__ == "__main__":
    unittest.main()
