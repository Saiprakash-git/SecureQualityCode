import sys


def read_fixed_bytes(size, prompt=None):
    if prompt:
        sys.stdout.write(prompt)
        sys.stdout.flush()

    buffer = bytearray(size)
    data = sys.stdin.buffer.readline()

    if data.endswith(b"\n"):
        data = data[:-1]
    if data.endswith(b"\r"):
        data = data[:-1]

    copy_len = min(size, len(data))
    buffer[:copy_len] = data[:copy_len]

    return buffer


def main():
    buffer_size = 20
    buffer = read_fixed_bytes(buffer_size, "Enter text (max 20 bytes): ")

    print("Buffer:", buffer)
    print("String:", buffer.rstrip(b"\x00").decode("utf-8", errors="replace"))

    return buffer


if __name__ == "__main__":
    main()