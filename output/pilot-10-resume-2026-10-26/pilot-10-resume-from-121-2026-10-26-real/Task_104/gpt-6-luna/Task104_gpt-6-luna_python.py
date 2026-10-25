import sys

BUFFER_SIZE = 100


def store_input(text, capacity=BUFFER_SIZE):
    if capacity < 1:
        raise ValueError("Buffer capacity must be at least 1.")

    encoded = text.encode("utf-8")
    stored = encoded[:capacity - 1]

    while stored:
        try:
            stored.decode("utf-8")
            break
        except UnicodeDecodeError:
            stored = stored[:-1]

    buffer = bytearray(capacity)
    buffer[:len(stored)] = stored
    buffer[len(stored)] = 0
    truncated = len(stored) < len(encoded)
    return buffer, truncated


def read_buffer(buffer):
    contents = bytes(buffer).split(b"\0", 1)[0]
    return contents.decode("utf-8")


def main():
    try:
        text = input("Enter text: ")
    except EOFError:
        return

    buffer, truncated = store_input(text)
    print(read_buffer(buffer))

    if truncated:
        print(f"Input was truncated to fit the {BUFFER_SIZE}-byte buffer.", file=sys.stderr)


if __name__ == "__main__":
    main()