import sys


def process_string(text):
    if not isinstance(text, str):
        raise TypeError("text must be a string")

    stripped = text.strip()
    words = stripped.split()

    return {
        "original_length": len(text),
        "stripped_length": len(stripped),
        "word_count": len(words),
        "uppercased": stripped.upper(),
        "lowercased": stripped.lower(),
        "reversed": stripped[::-1],
    }


def main():
    try:
        user_input = sys.stdin.readline()
    except KeyboardInterrupt:
        return

    if user_input.endswith("\n"):
        user_input = user_input[:-1]
    if user_input.endswith("\r"):
        user_input = user_input[:-1]

    result = process_string(user_input)

    for key, value in result.items():
        print(f"{key}: {value}")


if __name__ == "__main__":
    main()