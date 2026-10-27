import re
import sys

def find_matching_lines(file_path):
    pattern = re.compile(r'^\d+.*\.$', re.MULTILINE)
    with open(file_path, 'r') as file:
        content = file.read()
    return pattern.findall(content)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python script.py <filename>")
        sys.exit(1)
    matches = find_matching_lines(sys.argv[1])
    for line in matches:
        print(line)