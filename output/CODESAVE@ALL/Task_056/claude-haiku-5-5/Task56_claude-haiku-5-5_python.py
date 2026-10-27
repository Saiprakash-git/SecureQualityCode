import secrets
import hmac
import hashlib
import base64
import json
import time

SECRET_KEY = secrets.token_bytes(32)
DEFAULT_TTL_SECONDS = 3600


def _b64encode(data: bytes) -> str:
    return base64.urlsafe_b64encode(data).decode("ascii").rstrip("=")


def _b64decode(data: str) -> bytes:
    padding = "=" * (-len(data) % 4)
    return base64.urlsafe_b64decode(data + padding)


def _sign(message: bytes, key: bytes) -> str:
    signature = hmac.new(key, message, hashlib.sha256).digest()
    return _b64encode(signature)


def generate_token(user_id: str, key: bytes = SECRET_KEY, ttl: int = DEFAULT_TTL_SECONDS) -> str:
    if not user_id:
        raise ValueError("user_id must be a non-empty string")
    now = int(time.time())
    payload = {
        "sub": str(user_id),
        "iat": now,
        "exp": now + int(ttl),
        "jti": secrets.token_hex(16),
    }
    payload_bytes = json.dumps(payload, separators=(",", ":"), sort_keys=True).encode("utf-8")
    payload_part = _b64encode(payload_bytes)
    signature = _sign(payload_part.encode("ascii"), key)
    return f"{payload_part}.{signature}"


def verify_token(token: str, key: bytes = SECRET_KEY) -> dict:
    try:
        payload_part, signature = token.split(".", 1)
    except (ValueError, AttributeError):
        raise ValueError("Malformed token")
    expected = _sign(payload_part.encode("ascii"), key)
    if not hmac.compare_digest(signature, expected):
        raise ValueError("Invalid token signature")
    payload = json.loads(_b64decode(payload_part).decode("utf-8"))
    if int(time.time()) >= payload.get("exp", 0):
        raise ValueError("Token has expired")
    return payload


if __name__ == "__main__":
    import sys

    user = sys.argv[1] if len(sys.argv) > 1 else "user123"
    token = generate_token(user)
    print("Generated token:", token)
    print("Verified payload:", verify_token(token))