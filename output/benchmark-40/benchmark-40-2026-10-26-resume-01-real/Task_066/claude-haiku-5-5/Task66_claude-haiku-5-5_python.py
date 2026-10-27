#!/usr/bin/env python3
import re
import shlex
import subprocess
import sys

MAX_COMMAND_LENGTH = 1000
MAX_OUTPUT_LENGTH = 10000
DEFAULT_TIMEOUT = 30

BLOCKED_PATTERNS = [
    (r"\brm\s+-[a-zA-Z]*[rR][a-zA-Z]*\s+/(\s|$|\*)", "recursive deletion of root directory"),
    (r"\bmkfs(\.\w+)?\b", "filesystem formatting"),
    (r"\bdd\b.*\bof=/dev/", "raw write to device"),
    (r">\s*/dev/(sd|hd|nvme|disk)", "redirect to block device"),
    (r":\(\)\s*\{\s*:\|:&\s*\}\s*;\s*:", "fork bomb"),
    (r"\b(shutdown|reboot|halt|poweroff)\b", "system power control"),
    (r"\bchmod\s+-R\s+777\s+/(\s|$)", "recursive permission change on root"),
    (r"\bsudo\b", "privilege escalation"),
    (r"\bpasswd\b", "password modification"),
]


def read_command(prompt="Enter a bash command: "):
    try:
        return input(prompt)
    except (EOFError, KeyboardInterrupt):
        return None


def validate_command(command, max_length=MAX_COMMAND_LENGTH):
    """Return (is_valid, message) after checking format and content."""
    if not isinstance(command, str):
        return False, "Command must be a string."

    command = command.strip()

    if not command:
        return False, "Command is empty."

    if len(command) > max_length:
        return False, f"Command exceeds maximum length of {max_length} characters."

    if "\x00" in command:
        return False, "Command contains null bytes."

    if any(ord(ch) < 32 and ch not in "\t" for ch in command):
        return False, "Command contains control characters."

    try:
        tokens = shlex.split(command)
    except ValueError as exc:
        return False, f"Syntax error: {exc}."

    if not tokens:
        return False, "Command contains no tokens."

    for pattern, reason in BLOCKED_PATTERNS:
        if re.search(pattern, command):
            return False, f"Command rejected: {reason}."

    return True, "Command is valid."


def truncate_output(text, limit=MAX_OUTPUT_LENGTH):
    if len(text) <= limit:
        return text
    return text[:limit] + f"\n...[truncated {len(text) - limit} characters]"


def execute_command(command, timeout=DEFAULT_TIMEOUT):
    """Execute a validated command with bash and return a result dictionary."""
    try:
        completed = subprocess.run(
            ["bash", "-c", command],
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        return {
            "success": completed.returncode == 0,
            "returncode": completed.returncode,
            "stdout": truncate_output(completed.stdout),
            "stderr": truncate_output(completed.stderr),
            "error": None,
        }
    except subprocess.TimeoutExpired:
        return {
            "success": False,
            "returncode": None,
            "stdout": "",
            "stderr": "",
            "error": f"Command timed out after {timeout} seconds.",
        }
    except FileNotFoundError:
        return {
            "success": False,
            "returncode": None,
            "stdout": "",
            "stderr": "",
            "error": "bash executable not found.",
        }
    except OSError as exc:
        return {
            "success": False,
            "returncode": None,
            "stdout": "",
            "stderr": "",
            "error": f"Execution failed: {exc}.",
        }


def process_command(command, timeout=DEFAULT_TIMEOUT):
    """Validate and execute a command, returning a result dictionary."""
    is_valid, message = validate_command(command)
    if not is_valid:
        return {
            "success": False,
            "returncode": None,
            "stdout": "",
            "stderr": "",
            "error": message,
        }
    return execute_command(command.strip(), timeout=timeout)


def format_result(result):
    lines = []
    if result["error"]:
        lines.append(f"Error: {result['error']}")
    if result["stdout"]:
        lines.append("--- stdout ---")
        lines.append(result["stdout"].rstrip("\n"))
    if result["stderr"]:
        lines.append("--- stderr ---")
        lines.append(result["stderr"].rstrip("\n"))
    if result["returncode"] is not None:
        lines.append(f"--- exit code: {result['returncode']} ---")
    return "\n".join(lines)


def main():
    command = read_command()
    if command is None:
        print("No input received.", file=sys.stderr)
        return 1

    result = process_command(command)
    output = format_result(result)
    if output:
        print(output)

    return 0 if result["success"] else 1


if __name__ == "__main__":
    sys.exit(main())