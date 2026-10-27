import sys

def concatenate_strings(strings):
    return ''.join(strings)

if __name__ == "__main__":
    strings = sys.stdin.read().splitlines()
    print(concatenate_strings(strings))