import datetime
import email.utils
import json
import logging
import os
import re
import sys
import tempfile
import threading
import time
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import run_experiment


class RunnerTests(unittest.TestCase):
    def setUp(self):
        self.logger = logging.getLogger("runner-tests")
        self.config = {
            "languages": ["python"],
            "max_attempts": 2,
            "retry_delay_seconds": 0,
            "max_tokens": 4096,
            "model": {"provider": "openrouter", "model_id": "test/model"},
        }
        self.task = {"task_number": 7, "prompt_description": "Write a login function."}

    def call_result(self, content, cost=0.01, model="test/model", response_id="resp-1", finish="stop"):
        return {
            "id": response_id,
            "model": model,
            "choices": [{"message": {"content": content}, "finish_reason": finish}],
            "usage": {"prompt_tokens": 10, "completion_tokens": 5, "total_cost": cost},
        }

    @patch("run_experiment.call_openrouter")
    def test_raw_source_is_saved_as_source(self, call):
        call.return_value = self.call_result("def login(user):\n    return True\n")
        result = run_experiment.generate(
            self.task, self.config, "Return raw Python.", self.logger, language="python"
        )
        self.assertEqual(result["source"], "def login(user):\n    return True")
        self.assertEqual(result["validation_status"], "raw_source")
        self.assertEqual(result["response_status"], "success")
        self.assertEqual(result["attempts"][0]["response_model_id"], "test/model")
        self.assertEqual(result["attempts"][0]["response_id"], "resp-1")
        self.assertEqual(result["attempts"][0]["finish_reason"], "stop")

    @patch("run_experiment.call_openrouter")
    def test_one_matching_fence_is_fallback(self, call):
        call.return_value = self.call_result("```python\ndef login(user):\n    return True\n```")
        result = run_experiment.generate(
            self.task, self.config, "Return raw Python.", self.logger, language="python"
        )
        self.assertEqual(result["source"], "def login(user):\n    return True")
        self.assertEqual(result["validation_status"], "fence_stripped")

    def test_language_validation_and_prose_are_distinct(self):
        self.assertEqual(
            run_experiment.normalize_source_response("1 + 2", "python")[1],
            "raw_source",
        )
        self.assertEqual(
            run_experiment.normalize_source_response("print('ok')", "python")[1],
            "raw_source",
        )
        self.assertEqual(
            run_experiment.normalize_source_response("def broken(:", "python")[1],
            "invalid_syntax",
        )
        self.assertEqual(
            run_experiment.normalize_source_response("Sorry, I cannot help.", "python")[1],
            "refusal",
        )
        for language in ("c", "cpp", "java"):
            self.assertEqual(
                run_experiment.normalize_source_response(
                    "This is an explanation of the requested program.", language
                )[1],
                "prose_response",
            )
        self.assertEqual(
            run_experiment.normalize_source_response("int main() { return 0; }", "c")[1],
            "heuristic_source",
        )

    @patch("run_experiment.call_openrouter")
    def test_empty_response_is_recorded(self, call):
        call.return_value = self.call_result("   ")
        result = run_experiment.generate(
            self.task, self.config, "Return raw Python.", self.logger, language="python"
        )
        self.assertEqual(result["response_status"], "empty_content")
        self.assertEqual(result["validation_status"], "empty_response")
        self.assertEqual(len(result["attempts"]), 2)
        self.assertEqual(call.call_count, 2)

    @patch("run_experiment.call_openrouter")
    def test_refusal_and_invalid_output_are_recorded(self, call):
        call.side_effect = [
            self.call_result("Sorry, I cannot provide that code.", 0.01, response_id="refusal"),
            self.call_result("This is an explanation of the requested program.", 0.02, response_id="invalid"),
        ]
        result = run_experiment.generate(
            self.task, self.config, "Return raw Python.", self.logger, language="python"
        )
        self.assertEqual(result["response_status"], "validation_failed")
        self.assertEqual(result["validation_status"], "prose_response")
        self.assertEqual(result["cost_usd"], 0.03)
        self.assertEqual(len(result["attempts"]), 2)
        self.assertEqual(result["attempts"][0]["validation_status"], "refusal")

    @patch("run_experiment.call_openrouter")
    def test_budget_exhaustion_stops_before_retry(self, call):
        call.return_value = self.call_result("This is not source code.", 0.08)
        self.config["max_attempts"] = 3
        result = run_experiment.generate(
            self.task,
            self.config,
            "Return raw Python.",
            self.logger,
            language="python",
            spent_cost=0.95,
            budget_limit=1.0,
        )
        self.assertEqual(call.call_count, 1)
        self.assertEqual(result["cost_usd"], 0.08)
        self.assertEqual(result["response_status"], "budget_exhausted")
        self.assertEqual(len(result["attempts"]), 1)

    @patch("run_experiment.call_openrouter")
    def test_unknown_cost_stops_retries(self, call):
        call.return_value = self.call_result("This is not source code.", None)
        self.config["max_attempts"] = 3
        result = run_experiment.generate(
            self.task, self.config, "Return raw Python.", self.logger, language="python"
        )
        self.assertEqual(call.call_count, 1)
        self.assertTrue(result["cost_unknown"])
        self.assertEqual(result["response_status"], "cost_unknown")

    @patch("run_experiment.call_openrouter")
    def test_api_error_preserves_error_payload_and_cost(self, call):
        call.side_effect = run_experiment.OpenRouterResponseError(
            "OpenRouter HTTP 402: Payment Required",
            {"error": {"code": 402}, "usage": {"total_cost": 0.25}},
        )
        result = run_experiment.generate(
            self.task, self.config, "Return raw Python.", self.logger, language="python"
        )
        self.assertEqual(result["response_status"], "api_error")
        self.assertEqual(result["cost_usd"], 0.5)
        self.assertEqual(result["attempts"][0]["raw_response"]["error"]["code"], 402)

    @patch("run_experiment._response_content")
    @patch("run_experiment.call_openrouter")
    def test_processing_exception_does_not_double_count_usage(self, call, response_content):
        call.return_value = self.call_result("def login(user):\n    return True\n", 0.25)
        response_content.side_effect = RuntimeError("normalizer failed")
        self.config["max_attempts"] = 1
        result = run_experiment.generate(
            self.task, self.config, "Return raw Python.", self.logger, language="python"
        )
        self.assertEqual(result["response_status"], "processing_error")
        self.assertEqual(result["cost_usd"], 0.25)
        self.assertEqual(result["input_tokens"], 10)
        self.assertEqual(result["output_tokens"], 5)
        # The attempt keeps the known usage/cost for the audit trail...
        self.assertEqual(result["attempts"][0]["cost_usd"], 0.25)
        self.assertEqual(result["attempts"][0]["input_tokens"], 10)
        self.assertEqual(result["attempts"][0]["output_tokens"], 5)
        # ...and the totals above still count it exactly once (0.25, not 0.5).

    @patch("run_experiment.call_openrouter")
    def test_missing_choices_is_recorded(self, call):
        call.return_value = {"usage": {"total_cost": 0.01}}
        result = run_experiment.generate(
            self.task, self.config, "Return raw Python.", self.logger, language="python"
        )
        self.assertEqual(result["response_status"], "missing_choices")
        self.assertEqual(result["validation_status"], "not_attempted")

    def test_repeated_output_directory_is_rejected(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            output = Path(temp_dir) / "run"
            output.mkdir()
            (output / "old-ledger.jsonl").write_text("old", encoding="utf-8")
            with self.assertRaises(FileExistsError):
                run_experiment.prepare_output_dir(output)

    def test_multi_language_plan_has_400_combinations(self):
        tasks = [{"task_number": task_id} for task_id in range(20)]
        models = [{"model_id": f"model-{model_id}"} for model_id in range(5)]
        languages = ["python", "java", "c", "cpp"]
        plan = list(run_experiment.generation_plan(tasks, models, languages))
        self.assertEqual(len(plan), 400)
        self.assertEqual(
            len({(t["task_number"], m["model_id"], language) for m, t, language in plan}),
            400,
        )


class PilotConfigPreflightTests(unittest.TestCase):
    """Uses the real pilot config and the real dataset (read-only, no API calls)."""

    RUNNER_DIR = Path(__file__).resolve().parents[1]

    @classmethod
    def setUpClass(cls):
        cls.config = run_experiment.load_config(cls.RUNNER_DIR / "run_config_pilot.json")
        cls.all_tasks = run_experiment.load_all_tasks(cls.config["dataset_dirs"], cls.RUNNER_DIR)

    def test_select_tasks_rejects_unknown_mode(self):
        with self.assertRaises(ValueError):
            run_experiment.select_tasks(self.all_tasks, {"mode": "bogus"})

    def test_select_tasks_list_mode_is_supported(self):
        selected = run_experiment.select_tasks(self.all_tasks, {"mode": "list", "task_ids": [7, 1]})
        self.assertEqual([t["task_number"] for t in selected], [1, 7])

    def test_pilot_config_selects_20_unique_tasks(self):
        models = self.config["models"]
        selected = run_experiment.validate_preflight(self.all_tasks, self.config, models)
        numbers = [t["task_number"] for t in selected]
        self.assertEqual(len(numbers), 20)
        self.assertEqual(len(set(numbers)), 20)
        self.assertEqual(set(numbers), set(self.config["task_selection"]["orig_task_ids"]))

    def test_pilot_config_plans_exactly_400_unique_combinations(self):
        models = self.config["models"]
        selected = run_experiment.validate_preflight(self.all_tasks, self.config, models)
        plan = list(run_experiment.generation_plan(selected, models, self.config["languages"]))
        self.assertEqual(len(models), 5)
        self.assertEqual(len(self.config["languages"]), 4)
        self.assertEqual(len(plan), 400)
        keys = {(m["model_id"], t["task_number"], lang) for m, t, lang in plan}
        self.assertEqual(len(keys), 400)

    def test_preflight_rejects_wrong_expected_count(self):
        bad = dict(self.config, expected_task_count=21)
        with self.assertRaises(ValueError):
            run_experiment.validate_preflight(self.all_tasks, bad, self.config["models"])

    def test_preflight_rejects_wrong_expected_combinations(self):
        bad = dict(self.config, expected_combinations=399)
        with self.assertRaises(ValueError):
            run_experiment.validate_preflight(self.all_tasks, bad, self.config["models"])

    def test_pilot_budget_is_5_dollars(self):
        self.assertEqual(self.config["budget_usd_hard_limit"], 5.0)

    def test_pilot_run_id_is_not_existing_benchmark(self):
        self.assertNotEqual(self.config["run_id"], "pilot-90task")


class RegressionTests(unittest.TestCase):
    """Regression tests for bugs found in review."""

    def test_brace_regex_requires_an_actual_match(self):
        # Previously r"[*{};()]*" matched the empty string, so any text passed the
        # structure check. Text with keywords but no punctuation must now be rejected.
        self.assertFalse(run_experiment._looks_like_source("return value", "c")[0])
        self.assertFalse(run_experiment._looks_like_source("public class Foo", "java")[0])
        self.assertEqual(
            run_experiment.normalize_source_response("return value", "c")[1],
            "heuristic_rejected",
        )

    def test_brace_regex_still_accepts_real_code(self):
        self.assertTrue(run_experiment._looks_like_source("int main() { return 0; }", "c")[0])
        self.assertTrue(run_experiment._looks_like_source("public class A { }", "java")[0])


class DryRunTests(unittest.TestCase):
    """Dry run must make no API calls, call no persistence hooks, and write an empty ledger."""

    RUNNER_DIR = Path(__file__).resolve().parents[1]

    @patch("run_experiment.call_openrouter")
    def test_generate_dry_run_makes_no_api_call_and_no_attempts(self, call):
        seen = []
        result = run_experiment.generate(
            {"task_number": 7, "prompt_description": "x"},
            {"languages": ["python"], "max_attempts": 3},
            "instr", logging.getLogger("runner-tests"),
            model_cfg={"model_id": "test/model"}, language="python",
            dry_run=True, on_attempt=seen.append,
        )
        call.assert_not_called()
        self.assertEqual(seen, [])
        self.assertEqual(result["response_status"], "dry_run")
        self.assertEqual(result["cost_usd"], 0.0)

    @patch("run_experiment.call_openrouter")
    def test_main_dry_run_writes_empty_ledger_and_no_api_calls(self, call):
        config = json.loads((self.RUNNER_DIR / "run_config_pilot.json").read_text(encoding="utf-8"))
        with tempfile.TemporaryDirectory() as temp_dir:
            temp = Path(temp_dir)
            config["output_dir"] = str(temp / "out")
            config["run_id"] = "dryrun-test-001"
            config_path = temp / "dry.json"
            config_path.write_text(json.dumps(config), encoding="utf-8")
            root = logging.getLogger()
            handlers_before = list(root.handlers)
            try:
                with patch.object(sys, "argv", ["run_experiment.py", "--config", str(config_path), "--dry-run"]):
                    run_experiment.main()
            finally:
                # main() attaches a FileHandler; release it so Windows can delete the temp dir.
                for handler in list(root.handlers):
                    if handler not in handlers_before:
                        root.removeHandler(handler)
                        handler.close()
            run_dir = temp / "out" / "dryrun-test-001"
            call.assert_not_called()
            self.assertEqual((run_dir / "cost_ledger.jsonl").read_text(encoding="utf-8"), "")
            summary = json.loads((run_dir / "run_summary.json").read_text(encoding="utf-8"))
            self.assertTrue(summary["dry_run"])
            self.assertEqual(summary["total_cost_usd"], 0.0)
            self.assertEqual(summary["planned_combinations"], 400)


class ResumeTests(unittest.TestCase):
    """Resume from a combination index: no API calls for skipped work; spend carries over."""

    RUNNER_DIR = Path(__file__).resolve().parents[1]

    def _run_main(self, config_overrides, extra_args=None):
        config = json.loads((self.RUNNER_DIR / "run_config_pilot_10task_resume.json").read_text(encoding="utf-8"))
        config.update(config_overrides)
        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        temp = Path(temp_dir.name)
        config["output_dir"] = str(temp / "out")
        config_path = temp / "resume.json"
        config_path.write_text(json.dumps(config), encoding="utf-8")
        root = logging.getLogger()
        handlers_before = list(root.handlers)
        argv = ["run_experiment.py", "--config", str(config_path), "--dry-run"] + (extra_args or [])
        try:
            with patch.object(sys, "argv", argv):
                run_experiment.main()
        finally:
            for handler in list(root.handlers):
                if handler not in handlers_before:
                    root.removeHandler(handler)
                    handler.close()
        return temp / "out" / config["run_id"]

    def test_resume_plan_starts_at_task_43_python_claude(self):
        config = json.loads((self.RUNNER_DIR / "run_config_pilot_10task_resume.json").read_text(encoding="utf-8"))
        all_tasks = run_experiment.load_all_tasks(config["dataset_dirs"], self.RUNNER_DIR)
        selected = run_experiment.validate_preflight(all_tasks, config, config["models"])
        plan = list(run_experiment.generation_plan(selected, config["models"], config["languages"]))
        model, task, language = plan[120]  # combination 121 (1-based)
        self.assertEqual((task["task_number"], language, model["label"]), (43, "python", "claude-haiku-5-5"))
        self.assertEqual(len(plan) - 120, 80)

    def test_resume_writes_only_80_placeholders_and_skips_earlier_work(self):
        run_dir = self._run_main({"start_combination": 121, "prior_spend_usd": 0.0, "run_id": "resume-test-a"})
        summary = json.loads((run_dir / "run_summary.json").read_text(encoding="utf-8"))
        self.assertEqual(summary["tasks_attempted"], 80)
        self.assertEqual(summary["start_combination"], 121)
        sources = [p for p in run_dir.rglob("*") if p.is_file() and p.suffix in {".py", ".java", ".c", ".cpp"}]
        self.assertEqual(len(sources), 80)
        self.assertFalse((run_dir / "Task_024").exists())  # Task 24 is before the resume point

    def test_invalid_start_is_rejected_before_output(self):
        with self.assertRaises(SystemExit):
            self._run_main({"start_combination": 0, "run_id": "resume-test-b"})

    def test_prior_spend_counts_toward_the_cap(self):
        # With the recorded spend already at the $5 cap, no combination may be scheduled.
        run_dir = self._run_main({"start_combination": 121, "prior_spend_usd": 5.0, "run_id": "resume-test-c"})
        summary = json.loads((run_dir / "run_summary.json").read_text(encoding="utf-8"))
        self.assertEqual(summary["tasks_attempted"], 0)
        self.assertTrue(summary["stopped_early"])
        self.assertEqual(summary["prior_spend_usd"], 5.0)

    def test_resume_config_excludes_the_problem_combinations(self):
        config = json.loads((self.RUNNER_DIR / "run_config_pilot_10task_resume.json").read_text(encoding="utf-8"))
        self.assertEqual(config["start_combination"], 121)
        self.assertEqual(config["budget_usd_hard_limit"], 5.0)
        self.assertEqual(config["prior_spend_usd"], 0.637065)
        self.assertNotEqual(config["run_id"], "pilot-10-haiku-flash-2026-10-25-real")


class GenerationOrderTests(unittest.TestCase):
    def test_plan_is_task_then_language_then_model(self):
        tasks = [{"task_number": 1}, {"task_number": 2}]
        models = [{"model_id": "m1"}, {"model_id": "m2"}]
        plan = [(m["model_id"], t["task_number"], lang)
                for m, t, lang in run_experiment.generation_plan(tasks, models, ["python", "c"])]
        self.assertEqual(plan[:4], [("m1", 1, "python"), ("m2", 1, "python"),
                                    ("m1", 1, "c"), ("m2", 1, "c")])
        self.assertEqual(plan[4], ("m1", 2, "python"))
        self.assertEqual(len(plan), 8)

    def test_budget_cutoff_gives_every_model_the_same_early_tasks(self):
        tasks = [{"task_number": n} for n in range(1, 6)]
        models = [{"model_id": f"m{i}"} for i in range(5)]
        plan = list(run_experiment.generation_plan(tasks, models, ["python"]))
        first_15 = plan[:15]  # 3 tasks worth of combinations
        for model in models:
            covered = {t["task_number"] for m, t, _ in first_15 if m["model_id"] == model["model_id"]}
            self.assertEqual(covered, {1, 2, 3})


class PersistenceTests(unittest.TestCase):
    def setUp(self):
        self.logger = logging.getLogger("runner-tests")
        self.config = {"languages": ["python"], "max_attempts": 2, "retry_delay_seconds": 0,
                       "max_tokens": 4096, "model": {"provider": "openrouter", "model_id": "test/model"}}
        self.task = {"task_number": 7, "prompt_description": "Write a login function."}

    def call_result(self, content, cost=0.01):
        return {"id": "r", "model": "test/model",
                "choices": [{"message": {"content": content}, "finish_reason": "stop"}],
                "usage": {"prompt_tokens": 1, "completion_tokens": 1, "total_cost": cost}}

    @patch("run_experiment.call_openrouter")
    def test_on_attempt_called_once_per_attempt(self, call):
        call.return_value = self.call_result("This is prose only.", 0.01)
        seen = []
        result = run_experiment.generate(self.task, self.config, "x", self.logger,
                                         language="python", on_attempt=seen.append)
        self.assertEqual(len(seen), len(result["attempts"]))
        self.assertEqual([a["attempt"] for a in seen], [1, 2])

    @patch("run_experiment.call_openrouter")
    def test_on_attempt_called_on_api_error(self, call):
        call.side_effect = run_experiment.OpenRouterResponseError("timeout", None)
        self.config["max_attempts"] = 1
        seen = []
        run_experiment.generate(self.task, self.config, "x", self.logger,
                                language="python", on_attempt=seen.append)
        self.assertEqual(len(seen), 1)
        self.assertEqual(seen[0]["status"], "api_error")

    def test_write_new_file_refuses_overwrite(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "a.py"
            run_experiment._write_new_file(path, "one")
            with self.assertRaises(FileExistsError):
                run_experiment._write_new_file(path, "two")
            self.assertEqual(path.read_text(encoding="utf-8"), "one")


VALID_PY = "def solve(x):\n    return x\n"


def ok_response(content=VALID_PY, cost=0.01, finish="stop", response_id="resp"):
    return {"id": response_id, "model": "m",
            "choices": [{"message": {"content": content}, "finish_reason": finish}],
            "usage": {"prompt_tokens": 3, "completion_tokens": 4, "total_cost": cost}}


def http_error(code, retry_after=None):
    return run_experiment.OpenRouterResponseError(
        f"OpenRouter HTTP {code}: err", {"error": {"code": code}},
        status_code=code, retry_after=retry_after)


class MainHarness(unittest.TestCase):
    """Runs main() against a tiny temporary dataset with call_openrouter mocked."""

    def setUp(self):
        temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(temp_dir.cleanup)
        self.temp = Path(temp_dir.name)
        data = self.temp / "data"
        data.mkdir()
        for n in (1, 2, 3, 4):
            task = {"task_number": n, "prompt_title": f"t{n}", "prompt_description": f"TASK-{n}", "tags": []}
            (data / f"promptDescription-{n:03d}.json").write_text(json.dumps({"tasks": [task]}), encoding="utf-8")
        self.base_config = {
            "languages": ["python"], "max_attempts": 3, "retry_delay_seconds": 0,
            "max_tokens": 64, "budget_usd_hard_limit": 5.0, "prior_spend_usd": 0.0,
            "dataset_dirs": [str(data)], "output_dir": str(self.temp / "out"),
            "concurrency": 1, "http_retry_max": 2,
            "rate_limits": {"default": {"max_concurrent": 1, "min_interval_seconds": 0}},
            "models": [{"label": "a", "model_id": "pa/a", "reserve_usd": 0.01},
                       {"label": "b", "model_id": "pb/b", "reserve_usd": 0.01},
                       {"label": "c", "model_id": "pc/c", "reserve_usd": 0.01}],
            "task_selection": {"mode": "list", "task_ids": [1, 2]},
        }

    def run_main(self, run_id, overrides=None, dry_run=False, expect_exit=False):
        config = json.loads(json.dumps(self.base_config))
        config["run_id"] = run_id
        config.update(overrides or {})
        path = self.temp / f"{run_id}.json"
        path.write_text(json.dumps(config), encoding="utf-8")
        argv = ["run_experiment.py", "--config", str(path)] + (["--dry-run"] if dry_run else [])
        root = logging.getLogger()
        before = list(root.handlers)
        try:
            with patch.object(sys, "argv", argv), patch.dict(os.environ, {"OPENROUTER_API_KEY": "test"}), \
                 patch("run_experiment._sleep_unless_stopped", return_value=True):
                if expect_exit:
                    with self.assertRaises(SystemExit):
                        run_experiment.main()
                    return None
                run_experiment.main()
        finally:
            for handler in list(root.handlers):
                if handler not in before:
                    root.removeHandler(handler)
                    handler.close()
        return self.temp / "out" / run_id

    @staticmethod
    def ledger(run_dir):
        lines = (run_dir / "cost_ledger.jsonl").read_text(encoding="utf-8").splitlines()
        return [json.loads(line) for line in lines if line.strip()]

    @staticmethod
    def summary(run_dir):
        return json.loads((run_dir / "run_summary.json").read_text(encoding="utf-8"))


def task_of(kwargs):
    return int(re.search(r"TASK-(\d+)", kwargs["user_prompt"]).group(1))


class ConcurrencyTests(MainHarness):
    def _tracking_call(self, delay=0.15):
        lock = threading.Lock()
        state = {"now": 0, "max": 0, "per": {}, "max_per": {}}

        def call(**kwargs):
            provider = kwargs["model_id"].split("/")[0]
            with lock:
                state["now"] += 1
                state["per"][provider] = state["per"].get(provider, 0) + 1
                state["max"] = max(state["max"], state["now"])
                state["max_per"][provider] = max(state["max_per"].get(provider, 0), state["per"][provider])
            time.sleep(delay)
            with lock:
                state["now"] -= 1
                state["per"][provider] -= 1
            return ok_response()
        return call, state

    def test_workers_run_in_parallel_within_limits(self):
        call, state = self._tracking_call()
        with patch("run_experiment.call_openrouter", side_effect=call):
            run_dir = self.run_main("par", {"concurrency": 3,
                                            "task_selection": {"mode": "list", "task_ids": [1, 2, 3, 4]}})
        self.assertGreaterEqual(state["max"], 2)
        self.assertLessEqual(state["max"], 3)
        self.assertTrue(all(v == 1 for v in state["max_per"].values()))
        rows = self.ledger(run_dir)
        self.assertEqual(len(rows), 12)
        self.assertEqual(sorted(r["combo_index"] for r in rows), list(range(1, 13)))
        self.assertEqual(len(list(run_dir.rglob("*.py"))), 12)
        self.assertEqual(self.summary(run_dir)["combinations_finished_overall"], 12)

    def test_same_provider_models_share_one_slot(self):
        call, state = self._tracking_call(0.1)
        models = [{"label": "x", "model_id": "p1/x"}, {"label": "y", "model_id": "p1/y"}]
        with patch("run_experiment.call_openrouter", side_effect=call):
            self.run_main("shared", {"concurrency": 3, "models": models})
        self.assertEqual(state["max_per"]["p1"], 1)


class HttpRetryTests(unittest.TestCase):
    def setUp(self):
        self.logger = logging.getLogger("runner-tests")
        self.config = {"max_attempts": 3, "retry_delay_seconds": 0, "http_retry_max": 2,
                       "model": {"model_id": "test/model", "reserve_usd": 0.1}}
        self.task = {"task_number": 7, "prompt_description": "x"}

    def gen(self, budget):
        return run_experiment.generate(self.task, self.config, "i", self.logger,
                                       language="python", budget=budget)

    @patch("run_experiment._sleep_unless_stopped", return_value=True)
    @patch("run_experiment.call_openrouter")
    def test_429_honours_retry_after_and_is_not_an_attempt(self, call, sleep):
        call.side_effect = [http_error(429, retry_after=7), ok_response(cost=0.02)]
        budget = run_experiment.BudgetTracker(10.0)
        result = self.gen(budget)
        self.assertEqual(result["response_status"], "success")
        self.assertEqual(len(result["attempts"]), 1)
        self.assertEqual(sleep.call_args_list[0].args[0], 7)
        self.assertEqual(result["attempts"][0]["transport_retries"][0]["status_code"], 429)
        self.assertAlmostEqual(budget.known, 0.02)
        self.assertEqual(budget.provisional, 0.0)
        self.assertEqual(budget.reserved, 0.0)

    @patch("run_experiment._sleep_unless_stopped", return_value=True)
    @patch("run_experiment.call_openrouter")
    def test_5xx_backs_off_exponentially_and_is_charged_provisionally(self, call, sleep):
        call.side_effect = [http_error(503), http_error(502), ok_response(cost=0.02)]
        budget = run_experiment.BudgetTracker(10.0)
        result = self.gen(budget)
        self.assertEqual(result["response_status"], "success")
        first, second = (c.args[0] for c in sleep.call_args_list)
        self.assertTrue(2.0 <= first <= 2.5 and 4.0 <= second <= 5.0)
        self.assertAlmostEqual(budget.provisional, 0.2)
        self.assertFalse(budget.unknown_cost)

    @patch("run_experiment._sleep_unless_stopped", return_value=True)
    @patch("run_experiment.call_openrouter")
    def test_persistent_429_leaves_combination_resumable(self, call, sleep):
        call.side_effect = http_error(429)
        result = self.gen(run_experiment.BudgetTracker(10.0))
        self.assertEqual(call.call_count, 3)  # first try + 2 backoff retries
        self.assertEqual(result["response_status"], "rate_limited")
        self.assertFalse(result["cost_unknown"])
        self.assertEqual(result["attempts"][0]["billing"], "not_billed")

    @patch("run_experiment._sleep_unless_stopped", return_value=True)
    @patch("run_experiment.call_openrouter")
    def test_persistent_5xx_is_bounded_and_charged(self, call, sleep):
        call.side_effect = http_error(503)
        budget = run_experiment.BudgetTracker(10.0)
        result = self.gen(budget)
        self.assertEqual(call.call_count, 3)  # bounded: 1 + http_retry_max, then the combination stops
        self.assertEqual(result["response_status"], "rate_limited")
        self.assertAlmostEqual(budget.provisional, 0.3)
        self.assertEqual(result["attempts"][0]["billing"], "provisional")
        self.assertTrue(all(c.args[0] <= 60 * 1.25 for c in sleep.call_args_list))

    @patch("run_experiment._sleep_unless_stopped", return_value=True)
    @patch("run_experiment.call_openrouter")
    def test_huge_retry_after_is_capped(self, call, sleep):
        call.side_effect = [http_error(429, retry_after=10_000), ok_response()]
        self.gen(run_experiment.BudgetTracker(10.0))
        self.assertEqual(sleep.call_args_list[0].args[0], 300.0)

    @patch("run_experiment.call_openrouter")
    def test_non_retryable_error_is_not_retried_with_backoff(self, call):
        call.side_effect = http_error(400)
        self.config["max_attempts"] = 1
        budget = run_experiment.BudgetTracker(10.0)
        result = self.gen(budget)
        self.assertEqual(call.call_count, 1)
        self.assertTrue(result["cost_unknown"])
        self.assertTrue(budget.unknown_cost)

    def test_parse_retry_after(self):
        self.assertEqual(run_experiment._parse_retry_after("5"), 5.0)
        self.assertIsNone(run_experiment._parse_retry_after("soon"))
        self.assertIsNone(run_experiment._parse_retry_after(None))
        future = datetime.datetime.now(datetime.timezone.utc) + datetime.timedelta(seconds=30)
        parsed = run_experiment._parse_retry_after(email.utils.format_datetime(future))
        self.assertTrue(20 <= parsed <= 31)


class BudgetTests(MainHarness):
    def test_concurrent_reservations_never_exceed_the_cap(self):
        budget = run_experiment.BudgetTracker(1.0, 0.0)
        results = []
        barrier = threading.Barrier(40)

        def worker():
            barrier.wait()
            results.append(budget.try_reserve(0.1))
        threads = [threading.Thread(target=worker) for _ in range(40)]
        for t in threads:
            t.start()
        for t in threads:
            t.join()
        self.assertEqual(sum(results), 10)
        self.assertLessEqual(budget.reserved, 1.0 + 1e-9)

    def test_in_flight_reservations_keep_total_under_cap(self):
        lock = threading.Lock()
        state = {"now": 0, "max": 0}

        def call(**kwargs):
            with lock:
                state["now"] += 1
                state["max"] = max(state["max"], state["now"])
            time.sleep(0.1)
            with lock:
                state["now"] -= 1
            return ok_response(cost=0.3)
        models = [{"label": n, "model_id": f"p{n}/{n}", "reserve_usd": 0.4} for n in "abc"]
        with patch("run_experiment.call_openrouter", side_effect=call):
            run_dir = self.run_main("cap", {"concurrency": 3, "models": models, "budget_usd_hard_limit": 1.0,
                                            "task_selection": {"mode": "list", "task_ids": [1, 2, 3, 4]}})
        summary = self.summary(run_dir)
        self.assertLessEqual(state["max"], 2)  # a third 0.4 reservation does not fit under $1
        self.assertLessEqual(summary["total_cost_usd"], 1.0)
        self.assertEqual(summary["stop_reason"], "budget_exhausted")

    def test_unknown_cost_pauses_dispatch_and_is_counted(self):
        with patch("run_experiment.call_openrouter", return_value=ok_response(cost=None)) as call:
            run_dir = self.run_main("unknown")
        summary = self.summary(run_dir)
        self.assertEqual(call.call_count, 1)
        self.assertEqual(summary["stop_reason"], "unknown_cost")
        self.assertAlmostEqual(summary["provisional_cost_usd"], 0.01)
        self.assertEqual(self.ledger(run_dir)[0]["billing"], "unknown")

    def test_unreconciled_spend_counts_toward_the_cap(self):
        with patch("run_experiment.call_openrouter") as call:
            run_dir = self.run_main("unrec", {"budget_usd_hard_limit": 1.0, "unreconciled_spend_usd": 0.995})
        call.assert_not_called()  # 0.995 + a 0.01 reservation does not fit under $1
        summary = self.summary(run_dir)
        self.assertEqual(summary["stop_reason"], "budget_exhausted")
        self.assertAlmostEqual(summary["cap_basis_usd"], 0.995)
        self.assertEqual(summary["total_cost_usd"], 0.0)  # recorded spend is reported separately

    def test_missing_api_key_stops_before_any_output(self):
        config = dict(self.base_config, run_id="nokey")
        path = self.temp / "nokey.json"
        path.write_text(json.dumps(config), encoding="utf-8")
        env = {k: v for k, v in os.environ.items() if k != "OPENROUTER_API_KEY"}
        with patch.object(sys, "argv", ["run_experiment.py", "--config", str(path)]), \
             patch.dict(os.environ, env, clear=True), patch("run_experiment.call_openrouter") as call:
            with self.assertRaises(SystemExit):
                run_experiment.main()
        call.assert_not_called()
        self.assertFalse((self.temp / "out" / "nokey").exists())


class ResumeAndDuplicateTests(MainHarness):
    def _interrupted_first_run(self):
        """Run 1 (one worker): a ok; b empty (billed) then persistent 429; c unknown cost -> pause."""
        def call(**kwargs):
            label = kwargs["model_id"].split("/")[1]
            if label == "a":
                return ok_response()
            if label == "b":
                if "previous response" in kwargs["user_prompt"]:
                    raise http_error(429)
                return ok_response(content="", finish="length")
            return ok_response(cost=None)
        with patch("run_experiment.call_openrouter", side_effect=call):
            return self.run_main("run1")

    def test_resume_requests_only_unfinished_combinations(self):
        run1 = self._interrupted_first_run()
        self.assertEqual(self.summary(run1)["stop_reason"], "unknown_cost")
        seen = []

        def call(**kwargs):
            seen.append((task_of(kwargs), kwargs["model_id"], "previous response" in kwargs["user_prompt"]))
            return ok_response()
        resume = {"resume_from_runs": [str(run1)], "concurrency": 3}
        with patch("run_experiment.call_openrouter", side_effect=call):
            run2 = self.run_main("run2", resume)
        self.assertEqual(sorted(seen), [(1, "pb/b", True), (2, "pa/a", False), (2, "pb/b", False), (2, "pc/c", False)])
        resumed_b = [r for r in self.ledger(run2) if r["task_id"] == 1]
        self.assertEqual([r["attempt"] for r in resumed_b], [2])
        summary = self.summary(run2)
        self.assertEqual(summary["skipped_complete"], 2)
        self.assertEqual(summary["combinations_not_finished"], 0)
        self.assertAlmostEqual(summary["prior_runs_cost_usd"], 0.02)

        with patch("run_experiment.call_openrouter") as call3:
            run3 = self.run_main("run3", {"resume_from_runs": [str(run1), str(run2)]})
        call3.assert_not_called()
        self.assertEqual(self.summary(run3)["tasks_attempted"], 0)

    def test_ctrl_c_saves_in_flight_work_and_resume_finishes_without_duplicates(self):
        calls = []
        started = []
        lock = threading.Lock()

        def call(**kwargs):
            with lock:
                started.append(1)
            time.sleep(0.3)
            with lock:
                calls.append((task_of(kwargs), kwargs["model_id"]))
            return ok_response()
        real_wait = run_experiment.wait
        state = {"raised": False}

        def interrupting_wait(*args, **kwargs):
            if not state["raised"]:
                while len(started) < 3:  # Ctrl+C arrives while three requests are in flight
                    time.sleep(0.01)
                state["raised"] = True
                raise KeyboardInterrupt
            return real_wait(*args, **kwargs)
        with patch("run_experiment.call_openrouter", side_effect=call), \
             patch("run_experiment.wait", side_effect=interrupting_wait):
            run1 = self.run_main("ctrlc1", {"concurrency": 3})
        summary1 = self.summary(run1)
        self.assertEqual(summary1["stop_reason"], "interrupted")
        self.assertEqual(len(calls), 3)  # the three in-flight requests finished and were saved
        self.assertEqual(len(self.ledger(run1)), 3)
        self.assertFalse((self.temp / "out" / ".runner.lock").exists())
        with patch("run_experiment.call_openrouter", side_effect=call):
            run2 = self.run_main("ctrlc2", {"resume_from_runs": [str(run1)], "concurrency": 3})
        self.assertEqual(len(calls), 6)
        self.assertEqual(len(set(calls)), 6)  # every (task, model) requested exactly once
        self.assertEqual(self.summary(run2)["combinations_not_finished"], 0)

    def test_lock_file_blocks_a_second_runner(self):
        out = self.temp / "out"
        out.mkdir()
        (out / ".runner.lock").write_text("{}", encoding="utf-8")
        with patch("run_experiment.call_openrouter") as call:
            self.run_main("locked", expect_exit=True)
        call.assert_not_called()
        self.assertTrue((out / ".runner.lock").exists())
        self.assertFalse((out / "locked").exists())

    def test_lock_covers_resumed_runs_even_with_another_output_dir(self):
        with patch("run_experiment.call_openrouter", return_value=ok_response()):
            run1 = self.run_main("lk1")
        (self.temp / "out" / ".runner.lock").write_text("{}", encoding="utf-8")  # a runner is resuming run1
        with patch("run_experiment.call_openrouter") as call:
            self.run_main("lk2", {"resume_from_runs": [str(run1)], "output_dir": str(self.temp / "other")},
                          expect_exit=True)
        call.assert_not_called()
        self.assertFalse((self.temp / "other" / "lk2").exists())
        self.assertFalse((self.temp / "other" / ".runner.lock").exists())  # partial locks released

    def test_keep_awake_only_during_real_runs(self):
        with patch("run_experiment.call_openrouter", return_value=ok_response()), \
             patch("run_experiment._keep_system_awake") as awake:
            self.run_main("awake1")
            self.assertEqual([c.args for c in awake.call_args_list], [(True,), (False,)])
            awake.reset_mock()
            self.run_main("awake2", dry_run=True)
            awake.assert_not_called()

    def test_lock_is_released_after_a_run(self):
        with patch("run_experiment.call_openrouter", return_value=ok_response()):
            self.run_main("free")
        self.assertFalse((self.temp / "out" / ".runner.lock").exists())

    def test_resume_refuses_success_without_source(self):
        with patch("run_experiment.call_openrouter", return_value=ok_response()):
            run1 = self.run_main("src1")
        next(run1.rglob("*.py")).unlink()
        with patch("run_experiment.call_openrouter") as call:
            self.run_main("src2", {"resume_from_runs": [str(run1)]}, expect_exit=True)
        call.assert_not_called()

    def test_resume_refuses_changed_model_id(self):
        with patch("run_experiment.call_openrouter", return_value=ok_response()):
            run1 = self.run_main("mid1")
        models = json.loads(json.dumps(self.base_config["models"]))
        models[0]["model_id"] = "pa/other"
        with patch("run_experiment.call_openrouter") as call:
            self.run_main("mid2", {"resume_from_runs": [str(run1)], "models": models}, expect_exit=True)
        call.assert_not_called()

    def test_resume_refuses_unclassified_unknown_cost_row(self):
        old = self.temp / "out" / "old"
        old.mkdir(parents=True)
        row = {"task_id": 1, "language": "python", "model_label": "a", "requested_model_id": "pa/a",
               "response_id": None, "cost_usd": None, "response_status": "api_error",
               "error": "IncompleteRead(110 bytes read)", "raw_response_path": "x"}
        (old / "cost_ledger.jsonl").write_text(json.dumps(row) + "\n", encoding="utf-8")
        with patch("run_experiment.call_openrouter") as call:
            self.run_main("old2", {"resume_from_runs": [str(old)]}, expect_exit=True)
        call.assert_not_called()

    def test_resume_refuses_mismatched_prior_spend(self):
        with patch("run_experiment.call_openrouter", return_value=ok_response()):
            run1 = self.run_main("ps1")
        with patch("run_experiment.call_openrouter") as call:
            self.run_main("ps2", {"resume_from_runs": [str(run1)], "prior_spend_usd": 1.0}, expect_exit=True)
        call.assert_not_called()


class ConfigAndTrackingTests(MainHarness):
    @patch("run_experiment.call_openrouter")
    def test_top_level_sampling_params_are_used_and_model_overrides_win(self, call):
        call.return_value = ok_response()
        config = {"max_attempts": 1, "temperature": 0.7, "top_p": 0.5, "max_tokens": 99}
        task = {"task_number": 1, "prompt_description": "x"}
        logger = logging.getLogger("runner-tests")
        run_experiment.generate(task, config, "i", logger, model_cfg={"model_id": "p/m"})
        self.assertEqual((call.call_args.kwargs["temperature"], call.call_args.kwargs["top_p"],
                          call.call_args.kwargs["max_tokens"]), (0.7, 0.5, 99))
        run_experiment.generate(task, config, "i", logger, model_cfg={"model_id": "p/m", "temperature": 0.1})
        self.assertEqual(call.call_args.kwargs["temperature"], 0.1)

    @patch("run_experiment.call_openrouter")
    def test_request_timeout_is_configurable_with_120s_default(self, call):
        call.return_value = ok_response()
        task = {"task_number": 1, "prompt_description": "x"}
        logger = logging.getLogger("runner-tests")
        run_experiment.generate(task, {"max_attempts": 1}, "i", logger, model_cfg={"model_id": "p/m"})
        self.assertEqual(call.call_args.kwargs["timeout"], 120.0)
        run_experiment.generate(task, {"max_attempts": 1, "request_timeout_seconds": 300}, "i", logger,
                                model_cfg={"model_id": "p/m"})
        self.assertEqual(call.call_args.kwargs["timeout"], 300.0)

    def test_ledger_flags_truncated_but_accepted_output(self):
        with patch("run_experiment.call_openrouter", return_value=ok_response(finish="length")):
            run_dir = self.run_main("trunc", {"task_selection": {"mode": "list", "task_ids": [1]}})
        rows = self.ledger(run_dir)
        self.assertTrue(all(r["truncated"] for r in rows))
        self.assertEqual(self.summary(run_dir)["truncated_accepted"], 3)


class Benchmark40ResumeTests(unittest.TestCase):
    """Read-only checks of the real benchmark-40 resume config against the real ledger."""

    RUNNER_DIR = Path(__file__).resolve().parents[1]

    @classmethod
    def setUpClass(cls):
        cls.original = run_experiment.load_config(cls.RUNNER_DIR / "run_config_benchmark_40task.json")
        cls.resume = run_experiment.load_config(cls.RUNNER_DIR / "run_config_benchmark_40task_resume.json")
        cls.run_dir = (cls.RUNNER_DIR / cls.resume["resume_from_runs"][0]).resolve()

    def test_resume_config_preserves_the_selection_and_settings(self):
        for key in ("languages", "temperature", "top_p", "max_tokens", "max_attempts",
                    "budget_usd_hard_limit", "prior_spend_usd", "task_selection", "dataset_dirs", "output_dir"):
            self.assertEqual(self.original[key], self.resume[key], key)
        strip = lambda models: [(m["label"], m["model_id"]) for m in models]
        self.assertEqual(strip(self.original["models"]), strip(self.resume["models"]))
        self.assertNotEqual(self.original["run_id"], self.resume["run_id"])

    def test_resume_plan_matches_the_real_ledger(self):
        if not (self.run_dir / "cost_ledger.jsonl").is_file():
            self.skipTest("benchmark-40 output not present")
        models = self.resume["models"]
        prior = run_experiment.load_prior_state([self.run_dir], models)
        all_tasks = run_experiment.load_all_tasks(self.resume["dataset_dirs"], self.RUNNER_DIR)
        selected = run_experiment.validate_preflight(all_tasks, self.resume, models)
        jobs, skipped = run_experiment.plan_jobs(selected, models, self.resume["languages"],
                                                 prior["combos"], self.resume["max_attempts"])
        self.assertEqual(sum(s["success"] for s in prior["combos"].values()), 152)
        self.assertEqual(skipped, 159)  # 152 saved + 7 that used all 3 attempts
        self.assertEqual(len(jobs), 641)
        first = jobs[0]
        self.assertEqual((first["combo_index"], first["task"]["task_number"], first["language"],
                          first["label"], first["start_attempt"]), (160, 18, "cpp", "qwen3-8-flash", 2))
        self.assertAlmostEqual(prior["known_cost_usd"] + self.resume["prior_spend_usd"], 2.080105, places=5)
        self.assertEqual(prior["provisional_cost_usd"], 0.0)
        # Cap basis includes the unrecorded smoke-run spend (see unreconciled_spend_note).
        self.assertGreaterEqual(self.resume["unreconciled_spend_usd"], 0.128197)

    def test_resume_02_plan_matches_both_real_ledgers(self):
        config = run_experiment.load_config(self.RUNNER_DIR / "run_config_benchmark_40task_resume_02.json")
        dirs = [(self.RUNNER_DIR / p).resolve() for p in config["resume_from_runs"]]
        if not (self.run_dir / "cost_ledger.jsonl").is_file():
            self.skipTest("benchmark-40 output not present")
        for d in dirs:  # a wrong path must fail here, not be skipped
            self.assertTrue((d / "cost_ledger.jsonl").is_file(), d)
        for key in ("languages", "temperature", "top_p", "max_tokens", "max_attempts", "budget_usd_hard_limit",
                    "prior_spend_usd", "unreconciled_spend_usd", "task_selection", "models", "concurrency"):
            self.assertEqual(config[key], self.resume[key], key)
        self.assertFalse(any(d.name == "benchmark-40-2026-10-26-resume-01" for d in dirs))  # dry-run folder
        models = config["models"]
        prior = run_experiment.load_prior_state(dirs, models)
        all_tasks = run_experiment.load_all_tasks(config["dataset_dirs"], self.RUNNER_DIR)
        selected = run_experiment.validate_preflight(all_tasks, config, models)
        jobs, skipped = run_experiment.plan_jobs(selected, models, config["languages"],
                                                 prior["combos"], config["max_attempts"])
        self.assertEqual((len(jobs), skipped), (382, 418))
        for job in jobs:  # nothing already finished is requested again
            state = prior["combos"].get(run_experiment.combination_key(
                job["task"]["task_number"], job["language"], job["label"]))
            self.assertFalse(state and (state["success"] or state["billed_attempts"] >= 3))
        starts = {j["combo_index"]: j["start_attempt"] for j in jobs}
        self.assertEqual((starts[280], starts[444], starts[458]), (3, 1, 1))  # the 3 timed-out requests
        basis = (config["prior_spend_usd"] + prior["known_cost_usd"] + prior["provisional_cost_usd"]
                 + config["unreconciled_spend_usd"])
        self.assertAlmostEqual(basis, 3.754125, places=5)

    def test_resume_03_plan_matches_all_three_real_ledgers(self):
        config = run_experiment.load_config(self.RUNNER_DIR / "run_config_benchmark_40task_resume_03.json")
        previous = run_experiment.load_config(self.RUNNER_DIR / "run_config_benchmark_40task_resume_02.json")
        changed = sorted(k for k in set(config) | set(previous) if config.get(k) != previous.get(k))
        self.assertEqual(changed, ["description", "request_timeout_seconds", "resume_from_runs", "run_id"])
        if not (self.run_dir / "cost_ledger.jsonl").is_file():
            self.skipTest("benchmark-40 output not present")
        dirs = [(self.RUNNER_DIR / p).resolve() for p in config["resume_from_runs"]]
        for d in dirs:
            self.assertTrue((d / "cost_ledger.jsonl").is_file(), d)
        self.assertFalse(any(d.name == "benchmark-40-2026-10-26-resume-01" for d in dirs))  # dry-run folder
        models = config["models"]
        prior = run_experiment.load_prior_state(dirs, models)
        all_tasks = run_experiment.load_all_tasks(config["dataset_dirs"], self.RUNNER_DIR)
        selected = run_experiment.validate_preflight(all_tasks, config, models)
        jobs, skipped = run_experiment.plan_jobs(selected, models, config["languages"],
                                                 prior["combos"], config["max_attempts"])
        self.assertEqual((len(jobs), skipped), (46, 754))
        self.assertEqual({j["label"] for j in jobs}, {"qwen3-8-flash"})
        for job in jobs:  # nothing already finished is requested again
            state = prior["combos"].get(run_experiment.combination_key(
                job["task"]["task_number"], job["language"], job["label"]))
            self.assertFalse(state and (state["success"] or state["billed_attempts"] >= 3))
        starts = {j["combo_index"]: j["start_attempt"] for j in jobs}
        self.assertEqual((starts[585], starts[350], starts[355]), (1, 1, 3))  # timeout, 429, 429 after 2 billed
        basis = (config["prior_spend_usd"] + prior["known_cost_usd"] + prior["provisional_cost_usd"]
                 + config["unreconciled_spend_usd"])
        self.assertAlmostEqual(basis, 5.89455, places=5)

    def test_reservations_cover_the_worst_observed_request(self):
        worst_seen = {"claude-haiku-5-5": 0.00207, "gpt-6-luna": 0.00353, "gemini-3-8-flash": 0.01548,
                      "mistral-large-4": 0.01772, "qwen3-8-flash": 0.00201}
        for model in self.resume["models"]:
            self.assertGreater(model["reserve_usd"], worst_seen[model["label"]], model["label"])
        self.assertEqual(self.resume["max_tokens"], 4096)  # the reservations assume this cap


if __name__ == "__main__":
    unittest.main()
