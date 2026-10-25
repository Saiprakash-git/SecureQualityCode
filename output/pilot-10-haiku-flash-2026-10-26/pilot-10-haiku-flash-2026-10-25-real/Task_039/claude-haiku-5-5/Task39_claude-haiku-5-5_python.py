import os
import subprocess
import sys


def read_with_system_command(filename):
    if os.name == "nt":
        command = ["cmd", "/c", "type", filename]
    else:
        command = ["cat", filename]
    result = subprocess.run(command, capture_output=True, text=True)
    return result.returncode, result.stdout, result.stderr


def main():
    filename = input("Enter a filename: ").strip()
    if not filename:
        print("No filename provided.")
        sys.exit(1)
    if not os.path.isfile(filename):
        print(f"Error: '{filename}' is not a valid file.")
        sys.exit(1)
    returncode, output, error = read_with_system_command(filename)
    if returncode == 0:
        print(output, end="")
    else:
        print(f"Error displaying file: {error.strip()}")
        sys.exit(returncode)


if __name__ == "__main__":
    main()