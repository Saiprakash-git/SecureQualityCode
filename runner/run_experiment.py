#!/usr/bin/env python3
"""
run_experiment.py
=================
Experiment runner for the MajorProject-VII benchmark.

Usage:
  python run_experiment.py [--config run_config.json] [--dry-run]

Reads run_config.json (or the file passed via --config) and:
  1. Selects task JSON files based on task_selection config
  2. Calls the configured LLM (via OpenRouter or direct provider)
  3. Extracts code per language and saves to output_dir/run_id/
  4. Writes a run manifest (JSON) and a cost ledger (JSONL) alongside

Supported provider: openrouter. Combinations run on a bounded worker pool
(config "concurrency") with per-provider pacing ("rate_limits"). A run can
resume from earlier run folders ("resume_from_runs"); finished combinations
are skipped by (task, language, model) key, not by position.
"""

import os
import sys
import json
import time
import random
import logging
import argparse
import datetime
import re
import threading
import email.utils
from collections import deque
from concurrent.futures import ThreadPoolExecutor, FIRST_COMPLETED, wait
from pathlib import Path
from typing import Any

# ── optional: load .env.local if present ─────────────────────────────────────
try:
    from dotenv import load_dotenv
    env_path = Path(__file__).parent / ".env.local"
    if env_path.exists():
        load_dotenv(env_path)
        print(f"[env] Loaded credentials from {env_path}")
except ImportError:
    pass  # python-dotenv not installed; rely on system env vars


# ─────────────────────────────────────────────────────────────────────────────
#  CONFIG LOADING
# ─────────────────────────────────────────────────────────────────────────────

def load_config(config_path: str) -> dict:
    with open(config_path, "r", encoding="utf-8") as f:
        return json.load(f)


# ─────────────────────────────────────────────────────────────────────────────
#  DATASET LOADING
# ─────────────────────────────────────────────────────────────────────────────

def load_all_tasks(dataset_dirs: list[str], script_dir: Path) -> dict[int, dict]:
    """Load all JSON task files from the configured dataset directories.
    Returns a dict keyed by task_number."""
    tasks: dict[int, dict] = {}
    for rel_dir in dataset_dirs:
        dir_path = (script_dir / rel_dir).resolve()
        if not dir_path.exists():
            print(f"[warn] Dataset directory not found: {dir_path}")
            continue
        for jf in sorted(dir_path.glob("*.json")):
            try:
                data = json.loads(jf.read_text(encoding="utf-8"))
                task = data["tasks"][0]
                tasks[task["task_number"]] = task
            except Exception as e:
                print(f"[warn] Could not parse {jf.name}: {e}")
    return tasks


def select_tasks(all_tasks: dict, selection: dict) -> list[dict]:
    mode = selection.get("mode", "list")
    if mode == "all":
        return sorted(all_tasks.values(), key=lambda t: t["task_number"])
    elif mode == "range":
        start = selection["range_start"]
        end = selection["range_end"]
        return [t for n, t in sorted(all_tasks.items()) if start <= n <= end]
    elif mode == "orig_id_list":
        # New mode: explicit list of original dataset task_number values
        ids = set(selection["orig_task_ids"])
        return [all_tasks[n] for n in sorted(ids) if n in all_tasks]
    elif mode == "list":
        ids = set(selection["task_ids"])
        return [all_tasks[n] for n in sorted(ids) if n in all_tasks]
    raise ValueError(f"Unsupported task_selection mode: {mode!r}")


def validate_preflight(all_tasks: dict, config: dict, model_configs: list[dict]) -> list[dict]:
    """Validate task selection and the expected pilot workload before API calls."""
    selection = config["task_selection"]
    if selection.get("mode") == "orig_id_list":
        requested = list(selection.get("orig_task_ids", []))
        missing = sorted(set(requested) - set(all_tasks))
        if missing:
            raise ValueError(f"Configured task IDs are missing from the dataset: {missing}")
        if len(requested) != len(set(requested)):
            raise ValueError("Configured task IDs contain duplicates.")

    selected = select_tasks(all_tasks, selection)
    expected_count = config.get("expected_task_count")
    if expected_count is not None and len(selected) != int(expected_count):
        raise ValueError(
            f"Expected exactly {expected_count} selected tasks, found {len(selected)}."
        )
    planned = len(selected) * len(model_configs) * len(config["languages"])
    expected_combinations = config.get("expected_combinations")
    if expected_combinations is not None and planned != int(expected_combinations):
        raise ValueError(
            f"Expected {expected_combinations} combinations, planned {planned}."
        )
    return selected


# ─────────────────────────────────────────────────────────────────────────────
#  LLM CLIENT
# ─────────────────────────────────────────────────────────────────────────────

OPENROUTER_BASE_URL = "https://openrouter.ai/api/v1"


class OpenRouterResponseError(RuntimeError):
    """An API response or transport failure with a preserved raw payload."""

    def __init__(self, message: str, raw_response: Any = None,
                 status_code: int | None = None, retry_after: float | None = None):
        super().__init__(message)
        self.raw_response = raw_response
        self.status_code = status_code
        self.retry_after = retry_after


class MissingApiKeyError(RuntimeError):
    """Raised before any request is sent, so it can never be billed."""


def _parse_retry_after(value: Any) -> float | None:
    """Parse a Retry-After header given as seconds or as an HTTP date."""
    if value is None or str(value).strip() == "":
        return None
    try:
        return max(0.0, float(value))
    except (TypeError, ValueError):
        pass
    try:
        when = email.utils.parsedate_to_datetime(str(value))
    except (TypeError, ValueError):
        return None
    if when is None:
        return None
    return max(0.0, (when - datetime.datetime.now(when.tzinfo)).total_seconds())

def build_instruction(languages: list[str]) -> str:
    """Build the generation instruction listing requested languages."""
    if len(languages) != 1:
        raise ValueError("Each generation request must contain exactly one language.")
    lang_display = languages[0].upper()
    return (
        "Instructions:\n"
        "1) Return ONLY raw source code. Do not use Markdown fences. "
        "Do not include explanations, labels, or prose.\n"
        f"2) Write one complete self-contained source file in {lang_display}. "
        "All functions should accept input as parameters and return output if any.\n"
        "3) Return only the source file contents, starting with the first source line.\n"
    )


def call_openrouter(model_id: str, system_prompt: str, user_prompt: str,
                    temperature: float, top_p: float, max_tokens: int,
                    timeout: float = 120) -> dict:
    """Call the OpenRouter API. Returns the full response dict."""
    import urllib.error
    import urllib.request

    api_key = os.environ.get("OPENROUTER_API_KEY", "")
    if not api_key:
        raise MissingApiKeyError("OPENROUTER_API_KEY environment variable is not set.")

    payload = json.dumps({
        "model": model_id,
        "messages": [
            {"role": "system", "content": system_prompt},
            {"role": "user",   "content": user_prompt},
        ],
        "temperature": temperature,
        "top_p": top_p,
        "max_tokens": max_tokens,
    }).encode("utf-8")

    req = urllib.request.Request(
        f"{OPENROUTER_BASE_URL}/chat/completions",
        data=payload,
        headers={
            "Content-Type": "application/json",
            "Authorization": f"Bearer {api_key}",
            "HTTP-Referer": "https://github.com/MajorProject-VII",
            "X-Title": "MajorProject-VII Benchmark",
        },
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            body = resp.read().decode("utf-8")
            try:
                return json.loads(body)
            except json.JSONDecodeError as exc:
                raise OpenRouterResponseError(
                    "OpenRouter returned a non-JSON response.", body
                ) from exc
    except urllib.error.HTTPError as exc:
        body = exc.read().decode("utf-8", errors="replace")
        try:
            raw_response = json.loads(body)
        except json.JSONDecodeError:
            raw_response = body
        retry_after = _parse_retry_after(exc.headers.get("Retry-After")) if exc.headers else None
        raise OpenRouterResponseError(
            f"OpenRouter HTTP {exc.code}: {exc.reason}", raw_response,
            status_code=exc.code, retry_after=retry_after,
        ) from exc


def _usage_values(response: Any) -> tuple[int, int, float | None]:
    usage = response.get("usage", {}) if isinstance(response, dict) else {}
    if not isinstance(usage, dict):
        usage = {}
    input_tokens = int(usage.get("prompt_tokens") or usage.get("input_tokens") or 0)
    output_tokens = int(usage.get("completion_tokens") or usage.get("output_tokens") or 0)
    reported_cost = usage.get("total_cost", usage.get("cost"))
    try:
        cost = float(reported_cost) if reported_cost is not None else None
    except (TypeError, ValueError):
        cost = None
    return input_tokens, output_tokens, cost


def _response_content(response: Any) -> tuple[str | None, str]:
    if not isinstance(response, dict):
        return None, "invalid_response"
    choices = response.get("choices")
    if not isinstance(choices, list) or not choices:
        return None, "missing_choices"
    first_choice = choices[0]
    if not isinstance(first_choice, dict):
        return None, "invalid_choice"
    message = first_choice.get("message")
    if not isinstance(message, dict):
        return None, "missing_message"
    content = message.get("content")
    if not isinstance(content, str) or not content.strip():
        return None, "empty_content"
    return content, "content_received"


def _raw_for_json(value: Any) -> Any:
    try:
        json.dumps(value)
        return value
    except (TypeError, ValueError):
        return repr(value)


# HTTP statuses retried with backoff. They do not use up a generation attempt.
RETRYABLE_HTTP_STATUS = {429, 500, 502, 503, 504}


class BudgetExhausted(Exception):
    """The next request's reservation would exceed the cumulative budget."""


class RunPaused(Exception):
    """Another request had an unknown cost; no new requests may be sent."""


class TransportRetriesExhausted(Exception):
    """HTTP 429/5xx persisted through every backoff retry, or a stop was requested."""

    def __init__(self, message: str, transport: list, billing: str, provisional_usd: float,
                 raw_response: Any = None):
        super().__init__(message)
        self.transport = transport
        self.billing = billing
        self.provisional_usd = provisional_usd
        self.raw_response = raw_response


class BudgetTracker:
    """Thread-safe cumulative budget.

    Each request reserves its worst-case cost before it is sent, so requests in
    flight on other workers are already counted when the next one is checked.
    `known` is reported cost; `provisional` holds reservations whose real cost
    is unknown (lost responses, 5xx), so they still count toward the cap.
    """

    def __init__(self, limit: float, spent: float = 0.0):
        self.limit = float(limit)
        self.known = float(spent)
        self.provisional = 0.0
        self.reserved = 0.0
        self.unknown_cost = False
        self._lock = threading.Lock()

    def committed(self) -> float:
        with self._lock:
            return self.known + self.provisional

    def has_room(self, amount: float = 0.0) -> bool:
        with self._lock:
            return self._fits(amount)

    def _fits(self, amount: float) -> bool:
        used = self.known + self.provisional + self.reserved
        return used < self.limit and used + amount <= self.limit

    def try_reserve(self, amount: float) -> bool:
        with self._lock:
            if self.unknown_cost or not self._fits(amount):
                return False
            self.reserved += amount
            return True

    def settle(self, reserved: float, actual: float | None) -> None:
        """Release a reservation. actual=None means the cost is unknown: the
        reservation is kept as provisional spend and the run must pause."""
        with self._lock:
            self.reserved -= reserved
            if actual is None:
                self.unknown_cost = True
                self.provisional += reserved
            else:
                self.known += actual

    def settle_provisional(self, reserved: float) -> None:
        """Release a reservation but keep it as spend (billing unclear, no pause)."""
        with self._lock:
            self.reserved -= reserved
            self.provisional += reserved


class ProviderLimiter:
    """Per-provider pacing: a cap on concurrent jobs, a minimum gap between
    request starts, and a cooldown shared by all workers after a 429/5xx."""

    def __init__(self, max_concurrent: int = 1, min_interval_seconds: float = 0.0):
        self.max_concurrent = max(1, int(max_concurrent))
        self.min_interval = max(0.0, float(min_interval_seconds))
        self.active = 0
        self._next_start = 0.0
        self._lock = threading.Lock()

    def has_capacity(self) -> bool:
        with self._lock:
            return self.active < self.max_concurrent

    def acquire_slot(self) -> None:
        with self._lock:
            self.active += 1

    def release_slot(self) -> None:
        with self._lock:
            self.active -= 1

    def wait_turn(self, stop_event: threading.Event | None = None) -> bool:
        """Block until this provider may start a request. False if stopped first."""
        while True:
            with self._lock:
                now = time.monotonic()
                delay = self._next_start - now
                if delay <= 0:
                    self._next_start = now + self.min_interval
                    return True
            if stop_event is not None and stop_event.is_set():
                return False
            time.sleep(min(delay, 1.0))

    def cooldown(self, seconds: float) -> None:
        with self._lock:
            self._next_start = max(self._next_start, time.monotonic() + seconds)


def _sleep_unless_stopped(seconds: float, stop_event: threading.Event | None) -> bool:
    """Sleep in short steps; return False as soon as a stop is requested."""
    deadline = time.monotonic() + seconds
    while True:
        remaining = deadline - time.monotonic()
        if remaining <= 0:
            return True
        if stop_event is not None and stop_event.is_set():
            return False
        time.sleep(min(remaining, 1.0))


def _send_request(model_cfg: dict, prompt: str, params: dict, config: dict,
                  budget: BudgetTracker, reserve: float, limiter: "ProviderLimiter | None",
                  logger, stop_event: threading.Event | None = None) -> tuple[Any, list]:
    """Send one generation request, retrying HTTP 429/5xx with backoff.

    Every HTTP request reserves `reserve` dollars first and settles it exactly
    once. Returns (raw_response, transport_retry_log).
    """
    transport: list[dict] = []
    retry_max = int(config.get("http_retry_max", 5))
    base = float(config.get("http_backoff_base_seconds", 2.0))
    cap = float(config.get("http_backoff_max_seconds", 60.0))
    retry_after_cap = float(config.get("http_retry_after_max_seconds", 300.0))
    provisional_total = 0.0
    for retry in range(retry_max + 1):
        if budget.unknown_cost:
            raise RunPaused("An earlier request has an unknown cost.")
        if not budget.try_reserve(reserve):
            if budget.unknown_cost:
                raise RunPaused("An earlier request has an unknown cost.")
            raise BudgetExhausted("Budget exhausted before the next request.")
        if limiter is not None and not limiter.wait_turn(stop_event):
            budget.settle(reserve, 0.0)
            raise TransportRetriesExhausted("Stop requested before the request was sent.",
                                            transport, "not_sent", provisional_total)
        try:
            raw_response = call_openrouter(
                model_id=model_cfg["model_id"],
                system_prompt="You are an expert software engineer. Return only raw source code.",
                user_prompt=prompt,
                temperature=params["temperature"],
                top_p=params["top_p"],
                max_tokens=params["max_tokens"],
                # A read timeout makes the cost unknown and pauses the run, so slow
                # reasoning models need a longer wait than the 120 s default.
                timeout=float(config.get("request_timeout_seconds", 120)),
            )
        except MissingApiKeyError:
            budget.settle(reserve, 0.0)
            raise
        except OpenRouterResponseError as exc:
            _, _, error_cost = _usage_values(exc.raw_response)
            if error_cost is not None or exc.status_code not in RETRYABLE_HTTP_STATUS:
                budget.settle(reserve, error_cost)  # None: unknown cost, the run pauses
                raise
            if exc.status_code == 429:
                # Rejected before generation; OpenRouter does not bill rate-limited requests.
                billing = "not_billed"
                budget.settle(reserve, 0.0)
            else:
                # 5xx billing is unclear: count the reservation as spend, keep running.
                billing = "provisional"
                budget.settle_provisional(reserve)
                provisional_total += reserve
            if exc.retry_after is not None:
                delay = min(exc.retry_after, retry_after_cap)
            else:
                delay = min(cap, base * (2 ** retry)) * random.uniform(1.0, 1.25)
            transport.append({
                "status_code": exc.status_code,
                "retry_after": exc.retry_after,
                "delay_seconds": round(delay, 3),
                "billing": billing,
                "error": str(exc),
                "raw_response": _raw_for_json(exc.raw_response),
            })
            if limiter is not None:
                limiter.cooldown(delay)
            if retry == retry_max:
                raise TransportRetriesExhausted(
                    f"{str(exc)} persisted after {retry_max} backoff retries.",
                    transport, "provisional" if provisional_total else "not_billed",
                    provisional_total, exc.raw_response,
                ) from exc
            logger.warning(f"  {model_cfg.get('label', model_cfg['model_id'])}: {exc}; "
                           f"backing off {delay:.1f}s (retry {retry + 1}/{retry_max})")
            if not _sleep_unless_stopped(delay, stop_event):
                raise TransportRetriesExhausted(
                    "Stop requested during backoff.", transport,
                    "provisional" if provisional_total else "not_billed",
                    provisional_total, exc.raw_response,
                ) from exc
            continue
        except Exception:
            budget.settle(reserve, None)  # transport failure after sending: cost unknown
            raise
        _, _, cost = _usage_values(raw_response)
        budget.settle(reserve, cost)
        return raw_response, transport
    raise AssertionError("unreachable")


def generation_params(config: dict, model_cfg: dict) -> dict:
    """Effective sampling parameters: model entry, then run config, then default."""
    return {
        "temperature": model_cfg.get("temperature", config.get("temperature", 0.2)),
        "top_p": model_cfg.get("top_p", config.get("top_p", 0.95)),
        "max_tokens": model_cfg.get("max_tokens", config.get("max_tokens", 4096)),
    }


def generate(task: dict, config: dict, instruction: str, logger,
             model_cfg: dict | None = None, language: str = "python",
             spent_cost: float = 0.0, budget_limit: float | None = None,
             dry_run=False, on_attempt=None, budget: BudgetTracker | None = None,
             limiter: ProviderLimiter | None = None, start_attempt: int = 1,
             stop_event: threading.Event | None = None) -> dict:
    """Generate one task/language/model result with auditable attempts.

    `start_attempt` > 1 continues a combination whose earlier billed attempts
    were recorded by a previous run.
    """
    model_cfg = model_cfg or config.get("model")
    if not model_cfg:
        raise RuntimeError("Configuration must define 'model' or non-empty 'models'.")

    max_attempts = int(config.get("max_attempts", config.get("retry_max", 3)))
    if max_attempts < 1:
        raise ValueError("max_attempts must be at least 1; it includes the initial request.")
    prompt = instruction + "\n\n" + task["prompt_description"]
    attempts: list[dict[str, Any]] = []
    total_input_tokens = 0
    total_output_tokens = 0
    total_cost = 0.0
    cost_unknown = False
    last_source = ""
    last_status = "not_started"
    last_validation = "not_attempted"
    last_error: str | None = None
    last_raw_response: Any = None

    if dry_run:
        source = f"# DRY RUN placeholder for task {task['task_number']}\n"
        return {
            "source": source,
            "input_tokens": 0,
            "output_tokens": 0,
            "cost_usd": 0.0,
            "cost_unknown": False,
            "error": None,
            "response_status": "dry_run",
            "validation_status": "raw_source",
            "raw_response": {"dry_run": True, "content": source},
            "attempts": [{"attempt": 1, "status": "dry_run", "cost_usd": 0.0}],
        }

    if budget is None:
        budget = BudgetTracker(budget_limit if budget_limit is not None else float("inf"), spent_cost)
    reserve = float(model_cfg.get("reserve_usd", config.get("reserve_usd_per_request", 0.0)))
    params = generation_params(config, model_cfg)
    provider = model_cfg.get("provider", "openrouter")
    for attempt in range(start_attempt, max_attempts + 1):
        if cost_unknown:
            last_status = "cost_unknown"
            last_error = "Cost became unknown; retries stopped for budget safety."
            break
        if stop_event is not None and stop_event.is_set():
            last_status = "paused"
            last_error = "Run stop requested before the next attempt."
            break

        attempt_prompt = prompt
        if attempt > 1:
            attempt_prompt += (
                "\n\nYour previous response was not valid source code. "
                "Return only the raw source file contents, with no Markdown fences "
                "or explanations."
            )
        raw_response: Any = None
        input_tokens = output_tokens = 0
        attempt_cost: float | None = None
        response_model_id = None
        response_id = None
        finish_reason = None
        response_received = False
        try:
            if provider != "openrouter":
                raise NotImplementedError(
                    f"Provider '{provider}' not implemented in this runner. Use 'openrouter'."
                )
            raw_response, transport = _send_request(
                model_cfg, attempt_prompt, params, config, budget, reserve,
                limiter, logger, stop_event,
            )
            last_raw_response = raw_response
            response_received = True
            input_tokens, output_tokens, attempt_cost = _usage_values(raw_response)
            total_input_tokens += input_tokens
            total_output_tokens += output_tokens
            if attempt_cost is None:
                cost_unknown = True
            else:
                total_cost += attempt_cost
            response_model_id = raw_response.get("model") if isinstance(raw_response, dict) else None
            response_id = raw_response.get("id") if isinstance(raw_response, dict) else None
            choices = raw_response.get("choices", []) if isinstance(raw_response, dict) else []
            if isinstance(choices, list) and choices and isinstance(choices[0], dict):
                finish_reason = choices[0].get("finish_reason")

            text, content_status = _response_content(raw_response)
            if text is None:
                validation_status = "empty_response" if content_status == "empty_content" else "not_attempted"
                source = ""
            else:
                source, validation_status = normalize_source_response(text, language)
            last_validation = validation_status
            if source:
                last_source = source
                last_status = "success"
                last_error = None
            else:
                last_status = content_status if text is None else "validation_failed"
                last_error = f"Source validation failed: {validation_status}"

            attempt_record = {
                "attempt": attempt,
                "status": last_status,
                "validation_status": validation_status,
                "response_model_id": response_model_id,
                "response_id": response_id,
                "finish_reason": finish_reason,
                "input_tokens": input_tokens,
                "output_tokens": output_tokens,
                "cost_usd": attempt_cost,
                "raw_response": _raw_for_json(raw_response),
                "error": last_error,
                "billing": "billed" if attempt_cost is not None else "unknown",
                "provisional_cost_usd": reserve if attempt_cost is None else None,
                "transport_retries": transport,
            }
            attempts.append(attempt_record)
            if cost_unknown and not source:
                last_status = "cost_unknown"
                last_error = "Cost became unknown; retries stopped for budget safety."
                attempt_record["status"] = last_status
                attempt_record["error"] = last_error
            if on_attempt is not None:
                on_attempt(attempt_record)
            if source or cost_unknown:
                break
        except BudgetExhausted as exc:
            last_status = "budget_exhausted"
            last_error = str(exc)
            break
        except RunPaused as exc:
            last_status = "paused"
            last_error = str(exc)
            break
        except TransportRetriesExhausted as exc:
            # Not a generation attempt: nothing was generated, so the
            # combination stays incomplete and can be resumed later.
            last_status = "rate_limited"
            last_error = str(exc)
            last_raw_response = _raw_for_json(exc.raw_response)
            attempts.append({
                "attempt": attempt,
                "status": last_status,
                "validation_status": "not_attempted",
                "response_model_id": None,
                "response_id": None,
                "finish_reason": None,
                "input_tokens": 0,
                "output_tokens": 0,
                "cost_usd": None,
                "raw_response": last_raw_response,
                "error": last_error,
                "billing": exc.billing,
                "provisional_cost_usd": exc.provisional_usd or None,
                "transport_retries": exc.transport,
            })
            if on_attempt is not None:
                on_attempt(attempts[-1])
            logger.warning(f"Task {task['task_number']} language {language}: {last_error}")
            break
        except Exception as exc:
            last_status = "processing_error" if response_received else "api_error"
            last_error = str(exc)
            if not response_received:
                last_raw_response = getattr(exc, "raw_response", None)
                error_input, error_output, error_cost = _usage_values(last_raw_response)
                total_input_tokens += error_input
                total_output_tokens += error_output
                if error_cost is None:
                    cost_unknown = True
                else:
                    total_cost += error_cost
            else:
                # Usage and cost were already added to the totals right after the
                # successful API return. Keep them on this attempt record for the
                # audit trail only; they are NOT added to the totals again here.
                error_input, error_output, error_cost = input_tokens, output_tokens, attempt_cost
            if isinstance(exc, MissingApiKeyError):
                billing = "not_sent"
            else:
                billing = "billed" if error_cost is not None else "unknown"
            attempts.append({
                "attempt": attempt,
                "status": last_status,
                "validation_status": "not_attempted",
                "response_model_id": response_model_id,
                "response_id": response_id,
                "finish_reason": finish_reason,
                "input_tokens": error_input,
                "output_tokens": error_output,
                "cost_usd": error_cost,
                "raw_response": _raw_for_json(last_raw_response),
                "error": last_error,
                "billing": billing,
                "provisional_cost_usd": reserve if billing == "unknown" else None,
            })
            if on_attempt is not None:
                on_attempt(attempts[-1])
            logger.warning(
                f"Task {task['task_number']} language {language} "
                f"attempt {attempt}/{max_attempts} failed: {last_error}"
            )
            if cost_unknown:
                break
        if attempt < max_attempts:
            time.sleep(config.get("retry_delay_seconds", 5))

    return {
        "source": last_source,
        "input_tokens": total_input_tokens,
        "output_tokens": total_output_tokens,
        "cost_usd": total_cost,
        "cost_unknown": cost_unknown,
        "error": last_error,
        "response_status": last_status,
        "validation_status": last_validation,
        "raw_response": _raw_for_json(last_raw_response),
        "attempts": attempts,
    }


# ─────────────────────────────────────────────────────────────────────────────
#  CODE EXTRACTION
# ─────────────────────────────────────────────────────────────────────────────

LANG_ALIASES = {
    "python": "py", "py": "py",
    "java": "java",
    "cpp": "cpp", "c++": "cpp",
    "c": "c",
}

REFUSAL_PATTERN = re.compile(
    r"^\s*(?:sorry|i(?:'|’)m sorry|i can(?:not|'t)|i am unable|as an ai|"
    r"here(?:'|’)s the code|here is the code|explanation\s*:)",
    re.IGNORECASE,
)
PROSE_PATTERN = re.compile(
    r"^\s*(?:this is|the following|here is|here's|sure[,!]|of course|"
    r"explanation\s*:|solution\s*:|the code above|this code)",
    re.IGNORECASE,
)


def _language_extension(language: str) -> str:
    return LANG_ALIASES.get(language.lower().strip(), language.lower().strip())


def _strip_matching_outer_fence(text: str, language: str) -> str | None:
    aliases = [alias for alias, ext in LANG_ALIASES.items() if ext == _language_extension(language)]
    language_pattern = "|".join(re.escape(alias) for alias in aliases)
    match = re.fullmatch(
        rf"\s*```(?:{language_pattern})\s*\r?\n(.*?)\r?\n```\s*",
        text,
        re.DOTALL | re.IGNORECASE,
    )
    return match.group(1) if match else None


def _looks_like_source(source: str, language: str) -> tuple[bool, str]:
    """Return (accepted, validation_mode), without classifying prose/refusals."""
    if not source.strip():
        return False, "empty"
    extension = _language_extension(language)
    if extension == "py":
        try:
            import ast
            ast.parse(source)
        except SyntaxError:
            return False, "parser_rejected"
        return True, "parser"
    if extension in {"java", "c", "cpp"}:
        # No compiler/parser is invoked here. Compilation remains a separate
        # evaluation stage, so this only establishes a conservative heuristic.
        has_structure = bool(re.search(r"[{};()]", source))
        has_code_tokens = bool(re.search(r"\b(?:class|struct|int|void|public|private|static|return|include)\b", source))
        return has_structure and has_code_tokens, "heuristic"
    return False, "unsupported_language"


def normalize_source_response(text: Any, language: str) -> tuple[str, str]:
    """Validate raw source, with one matching Markdown-fence fallback."""
    if not isinstance(text, str) or not text.strip():
        return "", "empty_response"
    if REFUSAL_PATTERN.match(text):
        return "", "refusal"
    if PROSE_PATTERN.match(text):
        return "", "prose_response"
    raw_source = text.strip()
    accepted, validation_mode = _looks_like_source(raw_source, language)
    if accepted:
        return raw_source, "raw_source" if validation_mode == "parser" else "heuristic_source"
    fenced_source = _strip_matching_outer_fence(raw_source, language)
    if fenced_source is not None:
        accepted, validation_mode = _looks_like_source(fenced_source, language)
        if accepted:
            return fenced_source.strip(), "fence_stripped" if validation_mode == "parser" else "heuristic_fence_stripped"
    if validation_mode == "parser_rejected":
        return "", "invalid_syntax"
    if validation_mode == "heuristic":
        return "", "heuristic_rejected"
    return "", "invalid_source"

def extract_code_blocks(text: Any, requested_languages: list[str] | None = None) -> dict[str, str]:
    """Extract complete configured-language fences from a model response."""
    if not isinstance(text, str) or not text.strip():
        return {}
    pattern = re.compile(
        r"```([A-Za-z0-9_+#-]+)\s*\r?\n(.*?)```",
        re.DOTALL | re.IGNORECASE,
    )
    requested = None
    if requested_languages:
        requested = {
            LANG_ALIASES.get(language.lower(), language.lower())
            for language in requested_languages
        }
    result: dict[str, str] = {}
    for match in pattern.finditer(text):
        lang_raw = match.group(1).lower().strip()
        code = match.group(2).strip()
        lang_ext = LANG_ALIASES.get(lang_raw)
        if lang_ext and (requested is None or lang_ext in requested):
            result[lang_ext] = code
    return result


def save_code(blocks: dict[str, str], out_dir: Path, stem: str, logger):
    """Write each language block to a file."""
    for ext, code in blocks.items():
        fpath = out_dir / f"{stem}.{ext}"
        fpath.write_text(code, encoding="utf-8")
        logger.info(f"  Saved: {fpath.name}")


def generation_plan(selected: list[dict], model_configs: list[dict],
                    languages: list[str]):
    """Yield one independent generation combination per task/language/model.

    Order is task -> language -> model, so if the budget stops the run early,
    every model has covered the same earliest tasks instead of one model
    getting all the coverage.
    """
    for task in selected:
        for language in languages:
            for model_cfg in model_configs:
                yield model_cfg, task, language


def _write_new_file(path: Path, text: str) -> None:
    """Write a file only if it does not exist yet, so outputs are never overwritten."""
    with path.open("x", encoding="utf-8") as fh:
        fh.write(text)


def prepare_output_dir(out_base: Path) -> None:
    """Require a fresh run directory so ledgers cannot be silently appended."""
    if out_base.exists() and any(out_base.iterdir()):
        raise FileExistsError(
            f"Run output already contains files: {out_base}. "
            "Choose a new run_id/output_dir."
        )
    out_base.mkdir(parents=True, exist_ok=True)


# ─────────────────────────────────────────────────────────────────────────────
#  MAIN
# ─────────────────────────────────────────────────────────────────────────────


def combination_key(task_num: int, language: str, model_label: str) -> tuple:
    return (int(task_num), language, model_label)


_NOT_SENT_ERRORS = ("OPENROUTER_API_KEY environment variable is not set",)


def _row_billing(row: dict) -> str:
    """Billing class of a ledger row. Rows written before the `billing` field
    existed are classified from the response id, cost and error text."""
    if row.get("billing"):
        return row["billing"]
    if row.get("response_id") or row.get("cost_usd") is not None:
        return "billed"
    error = str(row.get("error") or "")
    if error.startswith("OpenRouter HTTP 429"):
        return "not_billed"
    if any(error.startswith(text) for text in _NOT_SENT_ERRORS):
        return "not_sent"
    return "unknown"


def load_prior_state(run_dirs: list[Path], model_configs: list[dict]) -> dict:
    """Read earlier run folders (read-only) and summarise each combination.

    Returns {"combos": {key: {"success", "billed_attempts"}}, "known_cost_usd",
    "provisional_cost_usd", "rows", "summaries"}. Raises ValueError on anything
    that would make a resume unsafe: missing ledgers, a success row without its
    source file, a model ID that differs from the current config, or a pre-
    `billing` row whose cost cannot be classified.
    """
    model_ids = {m.get("label", m["model_id"]): m["model_id"] for m in model_configs}
    combos: dict[tuple, dict] = {}
    known = provisional = 0.0
    rows_read = 0
    summaries = []
    for run_dir in run_dirs:
        ledger = run_dir / "cost_ledger.jsonl"
        if not ledger.is_file():
            raise ValueError(f"No cost_ledger.jsonl in resume folder: {run_dir}")
        summary_path = run_dir / "run_summary.json"
        if summary_path.is_file():
            summaries.append(json.loads(summary_path.read_text(encoding="utf-8")))
        for line_no, line in enumerate(ledger.read_text(encoding="utf-8").splitlines(), start=1):
            if not line.strip():
                continue
            try:
                row = json.loads(line)
            except json.JSONDecodeError as exc:
                raise ValueError(f"{ledger}:{line_no} is not valid JSON") from exc
            rows_read += 1
            label = row["model_label"]
            if label in model_ids and row.get("requested_model_id") != model_ids[label]:
                raise ValueError(
                    f"{ledger}:{line_no}: {label} used {row.get('requested_model_id')}, "
                    f"config now says {model_ids[label]}. Model selections must not change."
                )
            billing = _row_billing(row)
            if billing == "unknown" and row.get("provisional_cost_usd") is None:
                raise ValueError(
                    f"{ledger}:{line_no}: request with unknown cost and no provisional amount. "
                    "Reconcile it on OpenRouter and add it to prior_spend_usd before resuming."
                )
            known += float(row.get("cost_usd") or 0.0)
            provisional += float(row.get("provisional_cost_usd") or 0.0)
            key = combination_key(row["task_id"], row["language"], label)
            state = combos.setdefault(key, {"success": False, "billed_attempts": 0})
            if billing == "billed":
                state["billed_attempts"] += 1
            if row.get("response_status") == "success":
                raw_path = Path(row["raw_response_path"])
                stem = re.sub(r"_attempt_\d+_response\.json$", "", raw_path.name)
                source = run_dir / raw_path.parent.parent.name / raw_path.parent.name / (
                    f"{stem}.{_language_extension(row['language'])}")
                if not source.is_file():
                    raise ValueError(f"{ledger}:{line_no}: success recorded but source missing: {source}")
                state["success"] = True
    return {"combos": combos, "known_cost_usd": known, "provisional_cost_usd": provisional,
            "rows": rows_read, "summaries": summaries}


def acquire_run_lock(lock_path: Path, run_id: str) -> None:
    """Create the shared lock file atomically; refuse if another run holds it."""
    try:
        fd = os.open(str(lock_path), os.O_CREAT | os.O_EXCL | os.O_WRONLY)
    except FileExistsError:
        raise FileExistsError(
            f"Lock file exists: {lock_path}. Another run may be using this output folder. "
            "If you are sure none is running (for example after a crash), delete the file."
        )
    with os.fdopen(fd, "w", encoding="utf-8") as fh:
        fh.write(json.dumps({"run_id": run_id, "pid": os.getpid(),
                             "started": datetime.datetime.now().isoformat()}))


class LedgerWriter:
    """Serialised, fsynced JSONL writer shared by all workers."""

    def __init__(self, path: Path):
        self._fh = path.open("x", encoding="utf-8")
        self._lock = threading.Lock()

    def write(self, entry: dict) -> None:
        with self._lock:
            self._fh.write(json.dumps(entry) + "\n")
            self._fh.flush()
            os.fsync(self._fh.fileno())

    def close(self) -> None:
        with self._lock:
            self._fh.close()


def plan_jobs(selected: list[dict], model_configs: list[dict], languages: list[str],
              prior_combos: dict, max_attempts: int, start_combination: int = 1) -> tuple[list, int]:
    """Every plan combination that is not finished yet, in plan order.

    A combination is finished when it has a saved source or has used all
    billed attempts. Unfinished ones continue at the next attempt number.
    """
    jobs = []
    skipped_complete = 0
    for combo_index, (model_cfg, task, language) in enumerate(
        generation_plan(selected, model_configs, languages), start=1
    ):
        if combo_index < start_combination:
            continue
        label = model_cfg.get("label", model_cfg["model_id"])
        state = prior_combos.get(combination_key(task["task_number"], language, label))
        billed = state["billed_attempts"] if state else 0
        if state and (state["success"] or billed >= max_attempts):
            skipped_complete += 1
            continue
        jobs.append({"combo_index": combo_index, "model_cfg": model_cfg, "task": task,
                     "language": language, "label": label, "start_attempt": billed + 1,
                     "provider": provider_key(model_cfg)})
    return jobs, skipped_complete


def _keep_system_awake(enable: bool) -> None:
    """Ask Windows not to idle-sleep while a paid run is active. Sleep drops the
    network, which turns in-flight requests into unknown-cost timeouts and
    pauses the run. Lid close or a manual Sleep still suspend the machine."""
    if sys.platform != "win32":
        return
    try:
        import ctypes
        es_continuous, es_system_required = 0x80000000, 0x00000001
        ctypes.windll.kernel32.SetThreadExecutionState(
            es_continuous | (es_system_required if enable else 0))
    except Exception:
        pass  # best effort: the run is still safe without it


def provider_key(model_cfg: dict) -> str:
    return model_cfg.get("rate_limit_key") or model_cfg["model_id"].split("/")[0]


def build_limiters(config: dict, model_configs: list[dict]) -> dict[str, ProviderLimiter]:
    limits = config.get("rate_limits", {})
    default = {"max_concurrent": 1,
               "min_interval_seconds": config.get("sleep_between_requests_seconds", 2)}
    default.update(limits.get("default", {}))
    limiters = {}
    for model_cfg in model_configs:
        key = provider_key(model_cfg)
        if key not in limiters:
            spec = dict(default, **limits.get(key, {}))
            limiters[key] = ProviderLimiter(spec["max_concurrent"], spec["min_interval_seconds"])
    return limiters


def main():
    parser = argparse.ArgumentParser(description="MajorProject-VII Benchmark Runner")
    parser.add_argument("--config", default="run_config.json", help="Path to run_config.json")
    parser.add_argument("--dry-run", action="store_true", help="Skip actual API calls; write placeholder files")
    args = parser.parse_args()

    script_dir = Path(__file__).parent.resolve()
    config_path = (script_dir / args.config).resolve()

    if not config_path.exists():
        print(f"[error] Config not found: {config_path}")
        sys.exit(1)

    config = load_config(config_path)
    model_configs = config.get("models") or ([config["model"]] if config.get("model") else [])
    if not model_configs:
        print("[error] Config must define 'model' or a non-empty 'models' list.")
        sys.exit(1)
    run_id = config["run_id"]
    now = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")

    # ── preflight: validate tasks and the planned workload before any output or API call ──
    all_tasks = load_all_tasks(config["dataset_dirs"], script_dir)
    try:
        selected = validate_preflight(all_tasks, config, model_configs)
    except ValueError as exc:
        print(f"[error] Preflight validation failed: {exc}")
        sys.exit(1)

    # start_combination (1-based, plan order) is kept for older configs.
    # resume_from_runs is the key-based resume: finished combinations are skipped.
    start_combination = int(config.get("start_combination", 1))
    prior_spend = float(config.get("prior_spend_usd", 0.0))
    planned_total = len(selected) * len(model_configs) * len(config["languages"])
    if not 1 <= start_combination <= planned_total:
        print(f"[error] start_combination must be between 1 and {planned_total}.")
        sys.exit(1)
    if prior_spend < 0:
        print("[error] prior_spend_usd must not be negative.")
        sys.exit(1)
    concurrency = int(config.get("concurrency", 1))
    if concurrency < 1:
        print("[error] concurrency must be at least 1.")
        sys.exit(1)
    max_attempts = int(config.get("max_attempts", config.get("retry_max", 3)))
    if not args.dry_run and not os.environ.get("OPENROUTER_API_KEY"):
        print("[error] OPENROUTER_API_KEY is not set; no request was sent.")
        sys.exit(1)

    # ── shared locks: the output folder and the folder of every resumed run, so two
    # runners (even with different output_dir values) cannot request the same work ──
    out_root = (script_dir / config["output_dir"]).resolve()
    out_root.mkdir(parents=True, exist_ok=True)
    lock_dirs = {out_root} | {(script_dir / p).resolve().parent for p in config.get("resume_from_runs", [])}
    held = []
    try:
        for lock_dir in sorted((d for d in lock_dirs if d.is_dir()), key=str):
            acquire_run_lock(lock_dir / ".runner.lock", run_id)
            held.append(lock_dir / ".runner.lock")
    except FileExistsError as exc:
        for path in held:
            path.unlink(missing_ok=True)
        print(f"[error] {exc}")
        sys.exit(1)
    if not args.dry_run:
        _keep_system_awake(True)
    try:
        _run(config, args, script_dir, model_configs, selected, all_tasks, run_id, now,
             start_combination, prior_spend, planned_total, concurrency, max_attempts, out_root)
    finally:
        if not args.dry_run:
            _keep_system_awake(False)
        for path in held:
            path.unlink(missing_ok=True)


def _run(config, args, script_dir, model_configs, selected, all_tasks, run_id, now,
         start_combination, prior_spend, planned_total, concurrency, max_attempts, out_root):
    resume_dirs = [(script_dir / p).resolve() for p in config.get("resume_from_runs", [])]
    out_base = (out_root / run_id).resolve()
    if out_base in resume_dirs:
        print("[error] run_id must be new; it cannot also be a resume_from_runs folder.")
        sys.exit(1)
    try:
        prior = load_prior_state(resume_dirs, model_configs) if resume_dirs else {
            "combos": {}, "known_cost_usd": 0.0, "provisional_cost_usd": 0.0, "rows": 0, "summaries": []}
    except ValueError as exc:
        print(f"[error] Resume check failed: {exc}")
        sys.exit(1)
    # Spend known to exist but missing from the ledgers counted above (crashed or
    # unrecorded requests). It counts toward the cap only.
    unreconciled = float(config.get("unreconciled_spend_usd", 0.0))
    if unreconciled < 0:
        print("[error] unreconciled_spend_usd must not be negative.")
        sys.exit(1)
    for summary in prior["summaries"]:
        if abs(float(summary.get("prior_spend_usd", prior_spend)) - prior_spend) > 1e-9:
            print(f"[error] prior_spend_usd {prior_spend} differs from {summary.get('prior_spend_usd')} "
                  f"in resumed run {summary.get('run_id')}; spend would be double counted.")
            sys.exit(1)

    jobs, skipped_complete = plan_jobs(selected, model_configs, config["languages"],
                                       prior["combos"], max_attempts, start_combination)

    # ── output directory ──────────────────────────────────────────────────────
    prepare_output_dir(out_base)

    # ── logging ───────────────────────────────────────────────────────────────
    log_file = out_base / f"run_{now}.log"
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(message)s",
        handlers=[logging.FileHandler(log_file, encoding="utf-8"), logging.StreamHandler()],
    )
    logger = logging.getLogger("runner")
    logger.info(f"Run ID: {run_id}  |  Dry-run: {args.dry_run}  |  Workers: {concurrency}")
    logger.info(f"Output: {out_base}")
    logger.info(f"Tasks loaded: {len(all_tasks)} total, {len(selected)} selected")
    if resume_dirs:
        logger.info(f"Resume: {prior['rows']} ledger rows read from {len(resume_dirs)} run(s); "
                    f"{skipped_complete} combinations already finished")
    if start_combination > 1:
        logger.info(f"Resuming at combination {start_combination}; earlier combinations are skipped (no API calls).")
    logger.info(f"To run now: {len(jobs)} of {planned_total} planned combinations")
    cap_basis = prior_spend + prior["known_cost_usd"] + prior["provisional_cost_usd"] + unreconciled
    logger.info(f"Spend counted toward the cap: ${cap_basis:.6f} "
                f"(prior {prior_spend} + resumed ledgers {prior['known_cost_usd']:.6f} "
                f"+ provisional {prior['provisional_cost_usd']:.6f} + unreconciled {unreconciled}) "
                f"of ${config.get('budget_usd_hard_limit', 999):.2f}")

    if not selected:
        logger.error("No tasks matched the selection criteria. Check run_config.json.")
        sys.exit(1)

    limiters = build_limiters(config, model_configs)

    # ── run manifest ──────────────────────────────────────────────────────────
    manifest = {
        "run_id": run_id,
        "start_time": now,
        "config": config,
        "model_ids": [m["model_id"] for m in model_configs],
        "generation_params": {m.get("label", m["model_id"]): generation_params(config, m)
                              for m in model_configs},
        "task_ids_selected": [t["task_number"] for t in selected],
        "planned_combinations": planned_total,
        "max_attempts": max_attempts,
        "retry_semantics": "max_attempts includes the initial request; HTTP 429/5xx backoff retries are not attempts",
        "concurrency": concurrency,
        "rate_limits": {k: {"max_concurrent": v.max_concurrent, "min_interval_seconds": v.min_interval}
                        for k, v in limiters.items()},
        "resume_from_runs": [str(p) for p in resume_dirs],
        "skipped_complete": skipped_complete,
        "combinations_to_run": [j["combo_index"] for j in jobs],
        "dry_run": args.dry_run,
    }
    (out_base / "run_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    ledger = LedgerWriter(out_base / "cost_ledger.jsonl")

    # The cap covers earlier runs, every finished request in this run, and the
    # reservation of every request in flight.
    budget_limit = config.get("budget_usd_hard_limit", 999)
    budget = BudgetTracker(budget_limit, prior_spend + prior["known_cost_usd"] + unreconciled)
    budget.provisional = prior["provisional_cost_usd"]
    stop_event = threading.Event()
    stop_reason = None
    counters = {"dispatched": 0, "finished": 0, "success": 0, "error": 0,
                "truncated_accepted": 0, "input_tokens": 0, "output_tokens": 0, "cost": 0.0}
    overall_done = skipped_complete + (start_combination - 1)

    def run_job(job: dict) -> dict:
        model_cfg, task, language, label = job["model_cfg"], job["task"], job["language"], job["label"]
        task_num = task["task_number"]
        safe_label = re.sub(r"[^A-Za-z0-9_.-]+", "_", label)
        task_dir = out_base / f"Task_{task_num:03d}" / safe_label
        task_dir.mkdir(parents=True, exist_ok=True)
        stem = f"Task{task_num}_{safe_label}_{language}"

        def persist_attempt(attempt_record: dict) -> None:
            """Save one finished attempt (raw response + ledger line) immediately."""
            attempt_number = attempt_record["attempt"]
            raw_path = task_dir / f"{stem}_attempt_{attempt_number:02d}_response.json"
            _write_new_file(raw_path, json.dumps(attempt_record, indent=2))
            ledger.write({
                "task_id": task_num,
                "prompt_title": task.get("prompt_title", ""),
                "tags": task.get("tags", []),
                "language": language,
                "model_label": label,
                "requested_model_id": model_cfg["model_id"],
                "response_model_id": attempt_record.get("response_model_id"),
                "response_id": attempt_record.get("response_id"),
                "finish_reason": attempt_record.get("finish_reason"),
                "truncated": attempt_record.get("finish_reason") == "length",
                "attempt": attempt_number,
                "combo_index": job["combo_index"],
                "input_tokens": attempt_record.get("input_tokens", 0),
                "output_tokens": attempt_record.get("output_tokens", 0),
                "cost_usd": attempt_record.get("cost_usd"),
                "billing": attempt_record.get("billing"),
                "provisional_cost_usd": attempt_record.get("provisional_cost_usd"),
                "transport_retries": len(attempt_record.get("transport_retries") or []),
                "response_status": attempt_record.get("status"),
                "validation_status": attempt_record.get("validation_status"),
                "error": attempt_record.get("error"),
                "raw_response_path": str(raw_path),
            })

        result = generate(
            task, config, build_instruction([language]), logger,
            model_cfg=model_cfg, language=language, dry_run=args.dry_run,
            on_attempt=persist_attempt, budget=budget, limiter=limiters[job["provider"]],
            start_attempt=job["start_attempt"], stop_event=stop_event,
        )
        if result["source"]:
            source_path = task_dir / f"{stem}.{_language_extension(language)}"
            _write_new_file(source_path, result["source"])
            result["source_path"] = source_path
        return result

    def finish(job: dict, result: dict) -> None:
        nonlocal stop_reason, overall_done
        counters["finished"] += 1
        counters["cost"] += result["cost_usd"]
        counters["input_tokens"] += result["input_tokens"]
        counters["output_tokens"] += result["output_tokens"]
        tag = f"#{job['combo_index']} {job['label']} Task {job['task']['task_number']} {job['language']}"
        if result["source"]:
            counters["success"] += 1
            overall_done += 1
            last = result["attempts"][-1] if result["attempts"] else {}
            if last.get("finish_reason") == "length":
                counters["truncated_accepted"] += 1
                logger.warning(f"  {tag}: accepted but finish_reason=length (possibly truncated)")
            logger.info(f"  Saved: {result['source_path'].name}")
        else:
            counters["error"] += 1
            used = job["start_attempt"] - 1 + sum(1 for a in result["attempts"] if a.get("billing") == "billed")
            if used >= max_attempts:
                overall_done += 1  # all attempts used: finished, without a source
            logger.warning(f"  {tag}: {result['response_status']} / {result['validation_status']}")
        logger.info(
            f"  [{counters['finished']}/{len(jobs)} this run | {overall_done}/{planned_total} finished overall] "
            f"spent ${budget.committed():.4f} (+${budget.reserved:.4f} in flight) / ${budget_limit:.2f}"
        )
        if result["cost_unknown"] and not args.dry_run:
            logger.error("Cost metadata was missing; pausing before another paid request.")
            stop_reason = stop_reason or "unknown_cost"
            stop_event.set()
        elif result["response_status"] == "budget_exhausted":
            stop_reason = stop_reason or "budget_exhausted"
            stop_event.set()

    pending = deque(jobs)
    active: dict = {}
    pool = ThreadPoolExecutor(max_workers=concurrency)
    try:
        while pending or active:
            while pending and len(active) < concurrency and not stop_event.is_set():
                job = next((j for j in pending if limiters[j["provider"]].has_capacity()), None)
                if job is None:
                    break
                reserve = float(job["model_cfg"].get("reserve_usd", config.get("reserve_usd_per_request", 0.0)))
                if budget.unknown_cost:
                    stop_reason = stop_reason or "unknown_cost"
                    stop_event.set()
                    break
                if not budget.has_room(0.0 if args.dry_run else reserve):
                    logger.warning(f"Budget limit ${budget_limit:.2f} reached before scheduling "
                                   f"#{job['combo_index']}. Stopping.")
                    stop_reason = stop_reason or "budget_exhausted"
                    stop_event.set()
                    break
                pending.remove(job)
                limiters[job["provider"]].acquire_slot()
                counters["dispatched"] += 1
                logger.info(
                    f"[{counters['dispatched']}/{len(jobs)}] #{job['combo_index']}/{planned_total} "
                    f"{job['label']} Task {job['task']['task_number']} {job['language']} "
                    f"(attempt {job['start_attempt']}+): {job['task'].get('prompt_title', '')[:50]}"
                )
                active[pool.submit(run_job, job)] = job
            if not active:
                break
            done, _ = wait(list(active), timeout=1.0, return_when=FIRST_COMPLETED)
            for future in done:
                job = active.pop(future)
                limiters[job["provider"]].release_slot()
                try:
                    finish(job, future.result())
                except Exception as exc:  # a bug or I/O failure in one job: stop safely
                    counters["error"] += 1
                    logger.exception(f"  #{job['combo_index']} failed inside the runner: {exc}")
                    stop_reason = stop_reason or "runner_error"
                    stop_event.set()
    except KeyboardInterrupt:
        stop_reason = "interrupted"
        stop_event.set()
        logger.warning("Interrupted: no new requests; waiting for in-flight requests to be saved...")
        for future in list(active):
            job = active.pop(future)
            try:
                finish(job, future.result())
            except Exception as exc:
                logger.exception(f"  #{job['combo_index']} failed while stopping: {exc}")
    finally:
        pool.shutdown(wait=True, cancel_futures=True)
        ledger.close()

    # ── final summary ─────────────────────────────────────────────────────────
    summary = {
        "run_id": run_id,
        "end_time": datetime.datetime.now().strftime("%Y%m%d_%H%M%S"),
        "tasks_attempted": counters["dispatched"],
        "planned_combinations": planned_total,
        "tasks_success": counters["success"],
        "tasks_error": counters["error"],
        "truncated_accepted": counters["truncated_accepted"],
        "total_input_tokens": counters["input_tokens"],
        "total_output_tokens": counters["output_tokens"],
        "start_combination": start_combination,
        "prior_spend_usd": prior_spend,
        "resume_from_runs": [str(p) for p in resume_dirs],
        "prior_runs_cost_usd": round(prior["known_cost_usd"], 6),
        "this_run_cost_usd": round(counters["cost"], 6),
        "total_cost_usd": round(prior_spend + prior["known_cost_usd"] + counters["cost"], 6),
        "provisional_cost_usd": round(budget.provisional, 6),
        "unreconciled_spend_usd": unreconciled,
        "cap_basis_usd": round(budget.committed(), 6),
        "skipped_complete": skipped_complete,
        "combinations_finished_overall": overall_done,
        "combinations_not_finished": planned_total - overall_done,
        "concurrency": concurrency,
        "stopped_early": stop_reason is not None,
        "stop_reason": stop_reason,
        "budget_note": "Each request reserves its worst-case cost (reserve_usd) before it is sent.",
        "dry_run": args.dry_run,
    }
    (out_base / "run_summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")

    logger.info("=" * 60)
    logger.info(f"DONE — {counters['success']}/{counters['dispatched']} artifacts succeeded"
                + (f" | stopped: {stop_reason}" if stop_reason else ""))
    logger.info(f"Finished overall: {overall_done}/{planned_total}")
    logger.info(f"Total tokens: {counters['input_tokens']} in / {counters['output_tokens']} out")
    logger.info(f"Total cost:   ${summary['total_cost_usd']:.4f} (provisional ${budget.provisional:.4f})")
    logger.info(f"Output dir:   {out_base}")
    logger.info("=" * 60)


if __name__ == "__main__":
    main()
