import sys
from typing import Optional, TextIO, Tuple


class FixedBuffer:
    def __init__(self, capacity: int) -> None:
        if capacity <= 0:
            raise ValueError("Buffer capacity must be a positive integer.")
        self.capacity: int = capacity
        self._storage: bytearray = bytearray(capacity)
        self._head: int = 0
        self._tail: int = 0
        self._count: int = 0

    def write(self, data: bytes) -> Tuple[int, int]:
        """Writes data into the fixed buffer up to remaining capacity.

        Returns (bytes_written, bytes_dropped).
        """
        written = 0
        dropped = 0
        for byte in data:
            if self._count < self.capacity:
                self._storage[self._tail] = byte
                self._tail = (self._tail + 1) % self.capacity
                self._count += 1
                written += 1
            else:
                dropped += 1
        return written, dropped

    def read(self, max_bytes: Optional[int] = None) -> bytes:
        """Reads and removes up to max_bytes from the buffer."""
        if max_bytes is None or max_bytes < 0:
            bytes_to_read = self._count
        else:
            bytes_to_read = min(max_bytes, self._count)

        result = bytearray(bytes_to_read)
        for i in range(bytes_to_read):
            result[i] = self._storage[self._head]
            self._head = (self._head + 1) % self.capacity
            self._count -= 1
        return bytes(result)

    def peek(self) -> bytes:
        """Returns the current buffer content without consuming it."""
        idx = self._head
        snapshot = bytearray(self._count)
        for i in range(self._count):
            snapshot[i] = self._storage[idx]
            idx = (idx + 1) % self.capacity
        return bytes(snapshot)

    def clear(self) -> None:
        """Resets the buffer to empty state."""
        self._head = 0
        self._tail = 0
        self._count = 0

    @property
    def count(self) -> int:
        return self._count

    @property
    def free_space(self) -> int:
        return self.capacity - self._count

    @property
    def is_full(self) -> bool:
        return self._count == self.capacity

    @property
    def is_empty(self) -> bool:
        return self._count == 0


def process_user_input(
    user_line: str,
    buffer: FixedBuffer
) -> Tuple[int, int, bytes]:
    """Encodes a user string, writes it safely into the buffer,

    and returns (bytes_written, bytes_dropped, current_buffer_view).
    """
    raw_data = user_line.encode("utf-8")
    written, dropped = buffer.write(raw_data)
    current_view = buffer.peek()
    return written, dropped, current_view


def drain_buffer(buffer: FixedBuffer, chunk_size: int = 16) -> str:
    """Reads all contents out of the buffer and decodes to text."""
    collected = bytearray()
    while not buffer.is_empty:
        chunk = buffer.read(chunk_size)
        collected.extend(chunk)
    return collected.decode("utf-8", errors="replace")


def run_interactive_session(
    capacity: int,
    in_stream: TextIO = sys.stdin,
    out_stream: TextIO = sys.stdout
) -> int:
    """Runs a command-based session handling user inputs into the fixed-size buffer."""
    buffer = FixedBuffer(capacity=capacity)
    out_stream.write(f"=== Fixed-Size Buffer Initialized (Capacity: {capacity} bytes) ===\n")
    out_stream.write("Commands: ':read' to flush buffer, ':peek' to view, ':clear' to reset, ':quit' to exit.\n")
    out_stream.write("Any other input will be pushed into the buffer safely.\n\n")
    out_stream.flush()

    total_inputs_processed = 0

    while True:
        out_stream.write(f"[{buffer.count}/{buffer.capacity} bytes used] > ")
        out_stream.flush()

        line = in_stream.readline()
        if not line:
            break

        stripped = line.rstrip("\r\n")

        if stripped == ":quit":
            break
        elif stripped == ":read":
            content = drain_buffer(buffer)
            out_stream.write(f"Flushed content ({len(content)} chars): {repr(content)}\n")
        elif stripped == ":peek":
            view = buffer.peek()
            out_stream.write(f"Buffer content: {repr(view.decode('utf-8', errors='replace'))}\n")
        elif stripped == ":clear":
            buffer.clear()
            out_stream.write("Buffer cleared.\n")
        else:
            payload = stripped + "\n"
            written, dropped, _ = process_user_input(payload, buffer)
            total_inputs_processed += 1
            out_stream.write(f"Stored {written} byte(s).")
            if dropped > 0:
                out_stream.write(f" Dropped {dropped} byte(s) due to buffer exhaustion!")
            out_stream.write("\n")

        out_stream.flush()

    out_stream.write("Session terminated.\n")
    out_stream.flush()
    return total_inputs_processed


if __name__ == "__main__":
    run_interactive_session(capacity=32)