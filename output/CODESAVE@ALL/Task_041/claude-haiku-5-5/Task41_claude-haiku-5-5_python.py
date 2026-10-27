import sys

MAX_INPUT_LENGTH = 1024


def validate_input(text, max_length=MAX_INPUT_LENGTH):
    if not isinstance(text, str):
        raise TypeError("Input must be a string.")
    if len(text) > max_length:
        raise ValueError(f"Input exceeds maximum length of {max_length} characters.")
    return text


def process_text(text):
    stripped = text.strip()
    return {
        "original_length": len(text),
        "stripped_length": len(stripped),
        "uppercase": stripped.upper(),
        "lowercase": stripped.lower(),
        "reversed": stripped[::-1],
        "word_count": len(stripped.split()),
        "character_count": len(stripped),
    }


def read_input(max_length=MAX_INPUT_LENGTH):
    try:
        raw = sys.stdin.readline(max_length + 2)
    except Exception as e:
        raise IOError(f"Failed to read input: {e}")
    if len(raw) > max_length + 1:
        raise ValueError(f"Input exceeds maximum length of {max_length} characters.")
    return raw.rstrip("\r\n")


def format_results(results):
    lines = []
    for key, value in results.items():
        lines.append(f"{key}: {value}")
    return "\n".join(lines)


def main():
    try:
        text = read_input()
        text = validate_input(text)
        results = process_text(text)
        print(format_results(results))
    except (ValueError, TypeError, IOError) as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())