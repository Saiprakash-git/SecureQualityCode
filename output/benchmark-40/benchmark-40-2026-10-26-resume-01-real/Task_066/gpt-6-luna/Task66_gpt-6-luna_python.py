import json
import os
import shlex
import shutil
import subprocess
import sys
from typing import Any, TextIO


_ALLOWED_COMMANDS = {
    "cat",
    "cut",
    "date",
    "echo",
    "false",
    "grep",
    "head",
    "ls",
    "pwd",
    "tail",
    "true",
    "uname",
    "uniq",
    "wc",
    "whoami",
}
_FORBIDDEN_ARGUMENT_CHARACTERS = set(";|&<>$`(){}[]*?!#\\~")


def _validate(command: str) -> list[str]:
    if not isinstance(command, str):
        raise ValueError("Command must be text.")
    if not command.strip():
        raise ValueError("Command cannot be empty.")
    if len(command) > 4096:
        raise ValueError("Command is too long.")
    if any(ord(character) < 32 or ord(character) == 127 for character in command):
        raise ValueError("Control characters are not allowed.")

    try:
        parts = shlex.split(command, posix=True)
    except ValueError as error:
        raise ValueError(f"Invalid command format: {error}") from error

    if not parts:
        raise ValueError("Command cannot be empty.")
    if len(parts) > 256:
        raise ValueError("Too many command arguments.")
    if parts[0] not in _ALLOWED_COMMANDS:
        raise ValueError(f"Command is not permitted: {parts[0]}")

    for argument in parts:
        if len(argument) > 1024:
            raise ValueError("An argument is too long.")
        if any(character in _FORBIDDEN_ARGUMENT_CHARACTERS for character in argument):
            raise ValueError("Shell operators, expansions, and glob patterns are not allowed.")

    if parts[0] == "date":
        for argument in parts[1:]:
            if argument in {"-s", "--set", "-f", "--file"} or argument.startswith(
                ("--set=", "--file=")
            ):
                raise ValueError("This date option is not permitted.")

    return parts


def execute_command(command: str, timeout: float = 5.0) -> dict[str, Any]:
    try:
        parts = _validate(command)
    except ValueError as error:
        return {"stdout": "", "stderr": str(error), "returncode": None}

    try:
        executable = shutil.which(parts[0])
        if executable is None:
            return {
                "stdout": "",
                "stderr": f"Command is unavailable: {parts[0]}",
                "returncode": None,
            }

        safe_timeout = min(max(float(timeout), 0.1), 30.0)
        result = subprocess.run(
            [executable, *parts[1:]],
            shell=False,
            capture_output=True,
            text=True,
            timeout=safe_timeout,
            check=False,
            env={"PATH": os.defpath, "LC_ALL": "C"},
        )
        return {
            "stdout": result.stdout,
            "stderr": result.stderr,
            "returncode": result.returncode,
        }
    except subprocess.TimeoutExpired:
        return {"stdout": "", "stderr": "Command timed out.", "returncode": None}
    except (OSError, ValueError) as error:
        return {"stdout": "", "stderr": str(error), "returncode": None}


def main(stdin: TextIO = sys.stdin, stdout: TextIO = sys.stdout) -> int:
    command = stdin.readline()
    result = execute_command(command.rstrip("\r\n"))
    stdout.write(json.dumps(result, ensure_ascii=False) + "\n")
    return 0 if result["returncode"] == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())