import sys
import re

def sort_integers(arr):
    return sorted(arr)

def parse_integers(text):
    return [int(num) for num in re.findall(r"-?\d+", text)]

def main():
    data = sys.stdin.read().strip()
    if not data:
        return
    arr = parse_integers(data)
    sorted_arr = sort_integers(arr)
    print(" ".join(map(str, sorted_arr)))

if __name__ == "__main__":
    main()