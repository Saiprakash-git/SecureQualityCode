class FixedBuffer:
    def __init__(self, capacity):
        if not isinstance(capacity, int) or capacity <= 0:
            raise ValueError("capacity must be a positive integer")
        self.capacity = capacity
        self._data = []

    def write(self, text):
        if not isinstance(text, str):
            raise TypeError("text must be a string")
        available = self.capacity - len(self._data)
        accepted = text[:available]
        self._data.extend(accepted)
        return len(accepted)

    def read(self):
        return "".join(self._data)

    def clear(self):
        self._data.clear()

    def is_full(self):
        return len(self._data) >= self.capacity

    def __len__(self):
        return len(self._data)


def read_into_buffer(buffer, prompt="> ", stop_word="quit"):
    while not buffer.is_full():
        try:
            line = input(prompt)
        except EOFError:
            break
        if line.strip() == stop_word:
            break
        written = buffer.write(line + "\n")
        if written < len(line) + 1:
            break
    return buffer.read()


def main(capacity=256):
    buf = FixedBuffer(capacity)
    print(f"Enter text (buffer size {capacity} characters). Type 'quit' to finish.")
    content = read_into_buffer(buf)
    print("--- Buffer contents ---")
    print(content, end="")
    print()
    print(f"Stored {len(buf)} of {buf.capacity} characters.")
    return content


if __name__ == "__main__":
    main()