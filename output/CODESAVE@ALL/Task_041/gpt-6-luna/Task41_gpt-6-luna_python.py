import sys

MAX_INPUT_LENGTH = 1024


def read_bounded_input(stream, max_length=MAX_INPUT_LENGTH):
    line = stream.readline(max_length + 2)
    if line.endswith("\n"):
        line = line[:-1]
        if line.endswith("\r"):
            line = line[:-1]

    if len(line) > max_length:
        return None

    return line


def process_text(text):
    if not isinstance(text, str):
        raise TypeError("text must be a string")
    if len(text) > MAX_INPUT_LENGTH:
        raise ValueError("input is too long")
    return text.upper()


def main():
    text = read_bounded_input(sys.stdin)
    if text is None:
        print(f"Input exceeds the maximum length of {MAX_INPUT_LENGTH} characters.", file=sys.stderr)
        return 1

    print(process_text(text))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())