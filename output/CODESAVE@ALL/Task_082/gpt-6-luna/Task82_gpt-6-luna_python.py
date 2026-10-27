import hashlib
import hmac
import secrets
from typing import Optional, Union

Salt = Union[str, bytes]


def hash_password(password: str, salt: Optional[Salt] = None) -> str:
    """Return a PBKDF2-SHA256 password hash that includes its salt."""
    if not isinstance(password, str):
        raise TypeError("password must be a string")

    salt_bytes = secrets.token_bytes(16) if salt is None else (
        salt.encode("utf-8") if isinstance(salt, str) else salt
    )
    if not isinstance(salt_bytes, bytes):
        raise TypeError("salt must be a string or bytes")
    if len(salt_bytes) < 16:
        raise ValueError("salt must be at least 16 bytes")

    iterations = 600_000
    digest = hashlib.pbkdf2_hmac(
        "sha256", password.encode("utf-8"), salt_bytes, iterations
    )
    return f"pbkdf2_sha256${iterations}${salt_bytes.hex()}${digest.hex()}"


def verify_password(password: str, encoded_hash: str) -> bool:
    """Check a password against the encoded value returned by hash_password."""
    try:
        algorithm, iterations_text, salt_hex, digest_hex = encoded_hash.split("$")
        if algorithm != "pbkdf2_sha256":
            return False

        iterations = int(iterations_text)
        if iterations <= 0:
            return False

        salt = bytes.fromhex(salt_hex)
        expected_digest = bytes.fromhex(digest_hex)
        actual_digest = hashlib.pbkdf2_hmac(
            "sha256", password.encode("utf-8"), salt, iterations
        )
        return hmac.compare_digest(actual_digest, expected_digest)
    except (AttributeError, TypeError, ValueError):
        return False