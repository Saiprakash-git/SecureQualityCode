import re
from typing import List


def find_matching_lines(file_path: str) -> List[str]:
    pattern = re.compile(r'^\d+.*\.$')
    matches: List[str] = []

    with open(file_path, "r", encoding="utf-8") as file:
        for line in file:
            cleaned_line = line.rstrip("\r\n")
            if pattern.match(cleaned_line):
                matches.append(cleaned_line)

    return matches


if __name__ == "__main__":
    import sys

    if len(sys.argv) > 1:
        for line in find_matching_lines(sys.argv[1]):
            print(line)