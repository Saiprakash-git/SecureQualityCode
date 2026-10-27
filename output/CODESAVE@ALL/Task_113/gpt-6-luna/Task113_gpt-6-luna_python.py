import sys

strings = [line.rstrip("\r\n") for line in sys.stdin]
sys.stdout.write("".join(strings))