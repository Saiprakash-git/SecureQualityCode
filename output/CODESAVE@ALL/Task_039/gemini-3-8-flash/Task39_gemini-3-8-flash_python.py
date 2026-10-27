import os
import subprocess
import sys


def display_file_contents(filename: str) -> int:
    if not os.path.isfile(filename):
        print(f"Error: File '{filename}' does not exist or is not a valid file.", file=sys.stderr)
        return 1

    if os.name == "nt":
        command = ["cmd", "/c", "type", filename]
    else:
        command = ["cat", filename]

    try:
        result = subprocess.run(command, check=True)
        return result.returncode
    except subprocess.CalledProcessError as e:
        print(f"Command failed with exit code {e.returncode}.", file=sys.stderr)
        return e.returncode
    except FileNotFoundError as e:
        print(f"Command execution error: {e}", file=sys.stderr)
        return 1


def main() -> None:
    filename = input("Enter the filename: ").strip()
    if filename:
        display_file_contents(filename)
    else:
        print("No filename provided.", file=sys.stderr)


if __name__ == "__main__":
    main()