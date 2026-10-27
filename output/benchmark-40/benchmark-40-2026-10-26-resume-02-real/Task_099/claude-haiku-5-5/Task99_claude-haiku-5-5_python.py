import re
import sys


def match_lines(lines):
    pattern = re.compile(r'^\s*\d.*\.\s*$')
    return [line.rstrip('\n') for line in lines if pattern.match(line.rstrip('\n'))]


def read_file(path):
    with open(path, 'r', encoding='utf-8') as f:
        return f.readlines()


def main(argv):
    if len(argv) < 2:
        print("Usage: python program.py <filename>")
        return 1
    try:
        lines = read_file(argv[1])
    except OSError as e:
        print(f"Error reading file: {e}")
        return 1
    for line in match_lines(lines):
        print(line)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))