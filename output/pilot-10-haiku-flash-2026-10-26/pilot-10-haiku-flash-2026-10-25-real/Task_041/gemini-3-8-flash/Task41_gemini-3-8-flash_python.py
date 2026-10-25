import sys


def read_bounded_input(prompt: str, max_length: int = 1024) -> str:
    """Reads input safely from stdin with an enforced maximum length limit."""
    sys.stdout.write(prompt)
    sys.stdout.flush()

    # Reading with a specified size limit prevents reading unbounded amounts of data into memory
    raw_input_data = sys.stdin.readline(max_length + 1)

    # Clean newline characters
    if raw_input_data.endswith("\n"):
        raw_input_data = raw_input_data[:-1]
    if raw_input_data.endswith("\r"):
        raw_input_data = raw_input_data[:-1]

    # Enforce truncation at maximum buffer boundary
    if len(raw_input_data) > max_length:
        return raw_input_data[:max_length]
    return raw_input_data


def process_string(text: str) -> dict:
    """Processes a validated string and generates metadata and transformed representations."""
    # Filter non-printable characters for security and safety
    sanitized_text = "".join(char for char in text if char.isprintable())

    return {
        "length": len(sanitized_text),
        "uppercase": sanitized_text.upper(),
        "reversed": sanitized_text[::-1],
        "words": len(sanitized_text.split()),
    }


def run_pipeline(buffer_limit: int = 256) -> int:
    """Executes the input-processing workflow and returns the status code."""
    prompt = f"Enter a string (maximum {buffer_limit} characters): "
    user_string = read_bounded_input(prompt, buffer_limit)
    processed_results = process_string(user_string)

    sys.stdout.write("\nProcessed Results:\n")
    for key, value in processed_results.items():
        sys.stdout.write(f"  {key}: {value}\n")

    return 0


if __name__ == "__main__":
    exit_code = run_pipeline(256)
    sys.exit(exit_code)