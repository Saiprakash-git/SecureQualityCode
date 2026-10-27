import re
import sys
from pathlib import Path


LINE_PATTERN = re.compile(r"^\d+.*\.$")


def matching_lines(text):
    return [line for line in text.splitlines() if LINE_PATTERN.fullmatch(line)]


def read_matching_lines(file_path):
    return matching_lines(Path(file_path).read_text())


def main(args):
    if len(args) != 1:
        raise SystemExit(f"Usage: {Path(sys.argv[0]).name} FILE")
    for line in read_matching_lines(args[0]):
        print(line)


if __name__ == "__main__":
    main(sys.argv[1:])