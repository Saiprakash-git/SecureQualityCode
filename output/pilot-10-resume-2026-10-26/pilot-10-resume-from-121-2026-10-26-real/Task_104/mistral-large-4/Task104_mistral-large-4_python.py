def create_buffer(size):
    if size <= 0:
        raise ValueError("Buffer size must be positive")
    return {
        'data': bytearray(size),
        'size': size,
        'length': 0
    }

def buffer_write(buffer, data):
    if isinstance(data, str):
        data = data.encode('utf-8')
    elif not isinstance(data, (bytes, bytearray)):
        raise TypeError("Data must be str, bytes, or bytearray")

    available = buffer['size'] - buffer['length']
    to_write = min(len(data), available)

    if to_write > 0:
        start = buffer['length']
        buffer['data'][start:start + to_write] = data[:to_write]
        buffer['length'] += to_write

    return to_write

def buffer_read(buffer):
    return bytes(buffer['data'][:buffer['length']])

def buffer_clear(buffer):
    buffer['length'] = 0
    return buffer

def buffer_is_full(buffer):
    return buffer['length'] >= buffer['size']

def buffer_remaining(buffer):
    return buffer['size'] - buffer['length']

def handle_user_input(buffer_size, user_input):
    buf = create_buffer(buffer_size)
    written = buffer_write(buf, user_input)
    content = buffer_read(buf)

    if isinstance(user_input, str):
        input_len = len(user_input.encode('utf-8'))
    else:
        input_len = len(user_input)

    return {
        'buffer': buf,
        'written': written,
        'content': content,
        'truncated': input_len > written,
        'is_full': buffer_is_full(buf)
    }

def main():
    buffer_size = 32
    print(f"Fixed buffer size: {buffer_size} bytes")

    try:
        user_input = input("Enter text: ")
    except EOFError:
        user_input = "default input"

    result = handle_user_input(buffer_size, user_input)

    print(f"Bytes written: {result['written']}")
    print(f"Buffer content: {result['content']}")
    print(f"Truncated: {result['truncated']}")
    print(f"Buffer full: {result['is_full']}")

if __name__ == "__main__":
    main()