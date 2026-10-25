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
        self.assertEqual(result["attempts"][0]["cost_usd"], None)

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


if __name__ == "__main__":
    unittest.main()
