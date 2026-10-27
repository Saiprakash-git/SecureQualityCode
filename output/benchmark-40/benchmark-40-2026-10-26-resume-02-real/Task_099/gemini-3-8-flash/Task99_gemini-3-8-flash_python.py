import re
import sys


def filter_lines(lines: list[str]) -> list[str]:
    pattern = re.compile(r'^\d+.*\.$')
    return [line for line in lines if pattern.match(line.rstrip('\r\n'))]


def process_file(file_path: str) -> list[str]:
    with open(file_path, 'r', encoding='utf-8') as file:
        return filter_lines(file.readlines())


def main(argv: list[str]) -> int:
    if len(argv) < 2:
        print("Usage: python script.py <filepath>", file=sys.stderr)
        return 1

    file_path = argv[1]
    try:
        matching_lines = process_file(file_path)
        for line in matching_lines:
            print(line.rstrip('\r\n'))
    except OSError as e:
        print(f"Error reading file: {e}", file=sys.stderr)
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv))