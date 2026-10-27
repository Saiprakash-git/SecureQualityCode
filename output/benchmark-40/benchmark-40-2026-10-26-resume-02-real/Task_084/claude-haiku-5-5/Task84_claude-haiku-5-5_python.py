import secrets
import uuid


def generate_session_id(length_bytes: int = 32) -> str:
    if not isinstance(length_bytes, int) or length_bytes <= 0:
        raise ValueError("length_bytes must be a positive integer")
    unique_part = uuid.uuid4().hex
    random_part = secrets.token_urlsafe(length_bytes)
    return f"{unique_part}-{random_part}"


if __name__ == "__main__":
    print(generate_session_id())