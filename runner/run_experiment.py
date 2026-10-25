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

Supported providers: openrouter, openai, anthropic, google, mistral
"""

import os
import sys
import json
import time
import logging
import argparse
import datetime
import re
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
    else:  # list
        ids = set(selection["task_ids"])
        return [all_tasks[n] for n in sorted(ids) if n in all_tasks]


# ─────────────────────────────────────────────────────────────────────────────
#  LLM CLIENT
# ─────────────────────────────────────────────────────────────────────────────

OPENROUTER_BASE_URL = "https://openrouter.ai/api/v1"


class OpenRouterResponseError(RuntimeError):
    """An API response or transport failure with a preserved raw payload."""

    def __init__(self, message: str, raw_response: Any = None):
        super().__init__(message)
        self.raw_response = raw_response

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
                    temperature: float, top_p: float, max_tokens: int) -> dict:
    """Call the OpenRouter API. Returns the full response dict."""
    import urllib.error
    import urllib.request

    api_key = os.environ.get("OPENROUTER_API_KEY", "")
    if not api_key:
        raise RuntimeError("OPENROUTER_API_KEY environment variable is not set.")

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
        with urllib.request.urlopen(req, timeout=120) as resp:
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
        raise OpenRouterResponseError(
            f"OpenRouter HTTP {exc.code}: {exc.reason}", raw_response
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


def generate(task: dict, config: dict, instruction: str, logger,
             model_cfg: dict | None = None, language: str = "python",
             spent_cost: float = 0.0, budget_limit: float | None = None,
             dry_run=False) -> dict:
    """Generate one task/language/model result with auditable attempts."""
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

    provider = model_cfg.get("provider", "openrouter")
    for attempt in range(1, max_attempts + 1):
        if cost_unknown:
            last_status = "cost_unknown"
            last_error = "Cost became unknown; retries stopped for budget safety."
            break
        if budget_limit is not None and spent_cost + total_cost >= budget_limit:
            last_status = "budget_exhausted"
            last_error = "Budget exhausted before the next attempt."
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
            raw_response = call_openrouter(
                model_id=model_cfg["model_id"],
                system_prompt="You are an expert software engineer. Return only raw source code.",
                user_prompt=attempt_prompt,
                temperature=model_cfg.get("temperature", 0.2),
                top_p=model_cfg.get("top_p", 0.95),
                max_tokens=model_cfg.get("max_tokens", config.get("max_tokens", 4096)),
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
            }
            attempts.append(attempt_record)
            if cost_unknown and not source:
                last_status = "cost_unknown"
                last_error = "Cost became unknown; retries stopped for budget safety."
                attempt_record["status"] = last_status
                attempt_record["error"] = last_error
            if source or cost_unknown:
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
                # Usage and cost were already counted immediately after the
                # successful API return; do not count them again here.
                error_input = error_output = 0
                error_cost = None
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
            })
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
    """Yield one independent generation combination per task/language/model."""
    for model_cfg in model_configs:
        for task in selected:
            for language in languages:
                yield model_cfg, task, language


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

    # ── output directory ──────────────────────────────────────────────────────
    out_base = (script_dir / config["output_dir"] / run_id).resolve()
    prepare_output_dir(out_base)

    # ── logging ───────────────────────────────────────────────────────────────
    log_file = out_base / f"run_{now}.log"
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(message)s",
        handlers=[logging.FileHandler(log_file, encoding="utf-8"), logging.StreamHandler()],
    )
    logger = logging.getLogger("runner")
    logger.info(f"Run ID: {run_id}  |  Dry-run: {args.dry_run}")
    logger.info(f"Output: {out_base}")

    # ── load and select tasks ─────────────────────────────────────────────────
    all_tasks = load_all_tasks(config["dataset_dirs"], script_dir)
    selected = select_tasks(all_tasks, config["task_selection"])
    logger.info(f"Tasks loaded: {len(all_tasks)} total, {len(selected)} selected")

    if not selected:
        logger.error("No tasks matched the selection criteria. Check run_config.json.")
        sys.exit(1)

    # ── run manifest ──────────────────────────────────────────────────────────
    manifest = {
        "run_id": run_id,
        "start_time": now,
        "config": config,
        "model_ids": [m["model_id"] for m in model_configs],
        "task_ids_selected": [t["task_number"] for t in selected],
        "planned_combinations": len(selected) * len(model_configs) * len(config["languages"]),
        "max_attempts": int(config.get("max_attempts", config.get("retry_max", 3))),
        "retry_semantics": "max_attempts includes the initial request",
        "dry_run": args.dry_run,
    }
    (out_base / "run_manifest.json").write_text(
        json.dumps(manifest, indent=2), encoding="utf-8"
    )

    # ── ledger file (one JSON line per task) ──────────────────────────────────
    ledger_path = out_base / "cost_ledger.jsonl"
    ledger_file = ledger_path.open("x", encoding="utf-8")

    budget_limit = config.get("budget_usd_hard_limit", 999)
    total_cost = 0.0
    total_input_tokens = 0
    total_output_tokens = 0
    success_count = 0
    error_count = 0
    attempted_count = 0

    # ── per-model/per-task/language loop ─────────────────────────────────────
    planned_combinations = len(selected) * len(model_configs) * len(config["languages"])
    total_attempts = planned_combinations
    stop_requested = False
    for model_cfg, task, language in generation_plan(selected, model_configs, config["languages"]):
        model_label = model_cfg.get("label", model_cfg["model_id"])
        task_num = task["task_number"]
        if total_cost >= budget_limit:
            logger.warning(
                f"Budget limit ${budget_limit:.2f} reached before scheduling "
                f"{model_label} Task {task_num} {language}. Stopping."
            )
            stop_requested = True
            break

        attempted_count += 1
        logger.info(
            f"[{attempted_count}/{total_attempts}] {model_label} "
            f"Task {task_num} {language}: {task.get('prompt_title', '')[:60]}"
        )
        result = generate(
            task,
            config,
            build_instruction([language]),
            logger,
            model_cfg=model_cfg,
            language=language,
            spent_cost=total_cost,
            budget_limit=budget_limit,
            dry_run=args.dry_run,
        )

        safe_model_label = re.sub(r"[^A-Za-z0-9_.-]+", "_", model_label)
        task_dir = out_base / f"Task_{task_num:03d}" / safe_model_label
        task_dir.mkdir(parents=True, exist_ok=True)
        stem = f"Task{task_num}_{safe_model_label}_{language}"
        if result["source"]:
            source_path = task_dir / f"{stem}.{_language_extension(language)}"
            source_path.write_text(result["source"], encoding="utf-8")
            logger.info(f"  Saved: {source_path.name}")
            success_count += 1
        else:
            error_count += 1
            logger.warning(
                f"  {model_label} Task {task_num} {language}: "
                f"{result['response_status']} / {result['validation_status']}"
            )

        for attempt_record in result["attempts"]:
            attempt_number = attempt_record["attempt"]
            raw_path = task_dir / f"{stem}_attempt_{attempt_number:02d}_response.json"
            raw_path.write_text(json.dumps(attempt_record, indent=2), encoding="utf-8")
            ledger_entry = {
                "task_id": task_num,
                "prompt_title": task.get("prompt_title", ""),
                "tags": task.get("tags", []),
                "language": language,
                "model_label": model_label,
                "requested_model_id": model_cfg["model_id"],
                "response_model_id": attempt_record.get("response_model_id"),
                "response_id": attempt_record.get("response_id"),
                "finish_reason": attempt_record.get("finish_reason"),
                "attempt": attempt_number,
                "input_tokens": attempt_record.get("input_tokens", 0),
                "output_tokens": attempt_record.get("output_tokens", 0),
                "cost_usd": attempt_record.get("cost_usd"),
                "response_status": attempt_record.get("status"),
                "validation_status": attempt_record.get("validation_status"),
                "error": attempt_record.get("error"),
                "raw_response_path": str(raw_path),
            }
            ledger_file.write(json.dumps(ledger_entry) + "\n")
            ledger_file.flush()

        total_cost += result["cost_usd"]
        total_input_tokens += result["input_tokens"]
        total_output_tokens += result["output_tokens"]
        logger.info(f"  Cost so far: ${total_cost:.4f} / ${budget_limit:.2f}")

        if result["cost_unknown"] and not args.dry_run:
            logger.error("Cost metadata was missing; stopping before another paid request.")
            stop_requested = True
            break
        if attempted_count < total_attempts and not args.dry_run:
            time.sleep(config.get("sleep_between_requests_seconds", 2))

    ledger_file.close()

    # ── final summary ─────────────────────────────────────────────────────────
    summary = {
        "run_id": run_id,
        "end_time": datetime.datetime.now().strftime("%Y%m%d_%H%M%S"),
        "tasks_attempted": attempted_count,
        "planned_combinations": planned_combinations,
        "tasks_success": success_count,
        "tasks_error": error_count,
        "total_input_tokens": total_input_tokens,
        "total_output_tokens": total_output_tokens,
        "total_cost_usd": round(total_cost, 6),
        "stopped_early": stop_requested,
        "budget_note": "A single in-flight request may exceed the remaining budget.",
        "dry_run": args.dry_run,
    }
    (out_base / "run_summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")

    logger.info("=" * 60)
    logger.info(f"DONE — {success_count}/{attempted_count} artifacts succeeded")
    logger.info(f"Total tokens: {total_input_tokens} in / {total_output_tokens} out")
    logger.info(f"Total cost:   ${total_cost:.4f}")
    logger.info(f"Output dir:   {out_base}")
    logger.info("=" * 60)


if __name__ == "__main__":
    main()
