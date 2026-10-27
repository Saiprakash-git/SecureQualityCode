import re
import subprocess
import sys
from typing import Any, Dict, Tuple


def validate_command(command: str) -> Tuple[bool, str]:
    if not isinstance(command, str):
        return False, "Command must be a string."

    trimmed = command.strip()
    if not trimmed:
        return False, "Command cannot be empty."

    if len(trimmed) > 2048:
        return False, "Command exceeds maximum allowed length of 2048 characters."

    dangerous_patterns = [
        r":\(\)\s*\{\s*:\s*\|\s*:\s*&\s*\}\s*;\s*:",
        r"\brm\s+(-[a-zA-Z]*r[a-zA-Z]*\s+|-[a-zA-Z]*f[a-zA-Z]*\s+)*(--no-preserve-root\s+)?/\s*$",
        r"\bmkfs\b",
        r"\bdd\s+if=.*\bof=/dev/",
        r">\s*/dev/sd[a-z]",
    ]

    for pattern in dangerous_patterns:
        if re.search(pattern, trimmed):
            return False, f"Command contains prohibited dangerous pattern: {pattern}"

    try:
        syntax_check = subprocess.run(
            ["bash", "-n", "-c", trimmed],
            capture_output=True,
            text=True,
            timeout=5,
        )
        if syntax_check.returncode != 0:
            err_msg = syntax_check.stderr.strip() or "Syntax error."
            return False, f"Bash syntax validation failed: {err_msg}"
    except subprocess.TimeoutExpired:
        return False, "Syntax validation timed out."
    except FileNotFoundError:
        return False, "Bash interpreter not found on this system."

    return True, "Validation successful."


def execute_command(command: str, timeout: int = 30) -> Dict[str, Any]:
    is_valid, validation_msg = validate_command(command)
    if not is_valid:
        return {
            "success": False,
            "returncode": -1,
            "stdout": "",
            "stderr": validation_msg,
        }

    try:
        result = subprocess.run(
            ["bash", "-c", command],
            capture_output=True,
            text=True,
            timeout=timeout,
        )
        return {
            "success": result.returncode == 0,
            "returncode": result.returncode,
            "stdout": result.stdout,
            "stderr": result.stderr,
        }
    except subprocess.TimeoutExpired:
        return {
            "success": False,
            "returncode": -1,
            "stdout": "",
            "stderr": f"Execution timed out after {timeout} seconds.",
        }
    except Exception as exc:
        return {
            "success": False,
            "returncode": -1,
            "stdout": "",
            "stderr": str(exc),
        }


def main() -> None:
    try:
        user_input = input("Enter a bash command to execute: ")
    except (EOFError, KeyboardInterrupt):
        print("\nOperation cancelled.")
        sys.exit(0)

    result = execute_command(user_input)

    print("\n--- Execution Result ---")
    print(f"Success: {result['success']}")
    print(f"Exit Code: {result['returncode']}")
    if result["stdout"]:
        print("STDOUT:")
        print(result["stdout"], end="" if result["stdout"].endswith("\n") else "\n")
    if result["stderr"]:
        print("STDERR:")
        print(result["stderr"], end="" if result["stderr"].endswith("\n") else "\n")


if __name__ == "__main__":
    main()