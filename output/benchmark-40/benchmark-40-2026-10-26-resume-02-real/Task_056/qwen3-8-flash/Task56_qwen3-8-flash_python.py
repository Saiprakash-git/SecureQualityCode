import base64
import hashlib
import hmac
import json
import secrets
import sys
import time
from typing import Any, Dict, List, Optional


def _normalize_secret(secret_key: Any) -> bytes:
    if isinstance(secret_key, bytes):
        secret = secret_key
    elif isinstance(secret_key, str):
        secret = secret_key.encode("utf-8")
    else:
        raise TypeError("secret_key must be str or bytes")

    if not secret:
        raise ValueError("secret_key must not be empty")

    return secret


def _b64url_encode(data: bytes) -> str:
    return base64.urlsafe_b64encode(data).rstrip(b"=").decode("ascii")


def _b64url_decode(value: str) -> bytes:
    padding = "=" * (-len(value) % 4)
    return base64.urlsafe_b64decode(value + padding)


def generate_auth_token(
    user_id: str,
    secret_key: Any,
    expires_in_seconds: int = 3600,
    additional_claims: Optional[Dict[str, Any]] = None,
) -> str:
    if not user_id:
        raise ValueError("user_id must not be empty")

    if expires_in_seconds <= 0:
        raise ValueError("expires_in_seconds must be positive")

    secret = _normalize_secret(secret_key)
    now = int(time.time())

    header = {
        "alg": "HS256",
        "typ": "JWT",
    }

    payload = {
        "sub": user_id,
        "iat": now,
        "exp": now + int(expires_in_seconds),
        "jti": secrets.token_urlsafe(16),
    }

    if additional_claims:
        payload.update(additional_claims)

    header_b64 = _b64url_encode(
        json.dumps(header, separators=(",", ":"), sort_keys=True).encode("utf-8")
    )
    payload_b64 = _b64url_encode(
        json.dumps(payload, separators=(",", ":"), sort_keys=True).encode("utf-8")
    )

    signing_input = f"{header_b64}.{payload_b64}"
    signature = _b64url_encode(
        hmac.new(secret, signing_input.encode("utf-8"), hashlib.sha256).digest()
    )

    return f"{signing_input}.{signature}"


def verify_auth_token(
    token: str,
    secret_key: Any,
    now: Optional[int] = None,
) -> Optional[Dict[str, Any]]:
    try:
        parts = token.split(".")
        if len(parts) != 3:
            return None

        header_b64, payload_b64, signature_b64 = parts

        header = json.loads(_b64url_decode(header_b64).decode("utf-8"))
        if header.get("alg") != "HS256":
            return None

        expected_signature = _b64url_encode(
            hmac.new(
                _normalize_secret(secret_key),
                f"{header_b64}.{payload_b64}".encode("utf-8"),
                hashlib.sha256,
            ).digest()
        )

        if not hmac.compare_digest(signature_b64, expected_signature):
            return None

        payload = json.loads(_b64url_decode(payload_b64).decode("utf-8"))

        current_time = int(time.time()) if now is None else int(now)
        expires_at = payload.get("exp")

        if not isinstance(expires_at, (int, float)) or expires_at <= current_time:
            return None

        return payload

    except Exception:
        return None


def main(args: List[str]) -> int:
    if len(args) != 3:
        print(
            "usage: token_generator.py <user_id> <secret_key> <expires_in_seconds>",
            file=sys.stderr,
        )
        return 1

    try:
        user_id = args[0]
        secret_key = args[1]
        expires_in_seconds = int(args[2])
        token = generate_auth_token(user_id, secret_key, expires_in_seconds)
        print(token)
        return 0
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))