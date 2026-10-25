import json
import logging
import sys
import tempfile
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


if __name__ == "__main__":
    unittest.main()
