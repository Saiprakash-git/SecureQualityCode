"""Safe smoke-test entry point that preserves the existing runner files."""

from __future__ import annotations

import re
import sys
from pathlib import Path

runner_dir = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(runner_dir))

import run_experiment as base_runner


def safe_extract_code_blocks(
    text: str | None, requested_languages: list[str] | None = None
) -> dict[str, str]:
    """Extract complete fenced code blocks without crashing on null responses."""
    if not isinstance(text, str) or not text.strip():
        return {}

    pattern = re.compile(r"```(\w+)\s*\n(.*?)```", re.DOTALL | re.IGNORECASE)
    blocks: dict[str, str] = {}
    for match in pattern.finditer(text):
        language = base_runner.LANG_ALIASES.get(match.group(1).lower().strip())
        requested = {
            base_runner.LANG_ALIASES.get(item.lower(), item.lower())
            for item in requested_languages
        } if requested_languages else None
        if language and (requested is None or language in requested):
            blocks[language] = match.group(2).strip()
    return blocks


_original_generate = base_runner.generate


def safe_generate(*args, **kwargs):
    result = _original_generate(*args, **kwargs)
    if not isinstance(result.get("source"), str):
        result["source"] = ""
        result["error"] = result.get("error") or "Model returned null or non-text content."
    return result


base_runner.extract_code_blocks = safe_extract_code_blocks
base_runner.generate = safe_generate


if __name__ == "__main__":
    base_runner.main()
