import base64
import hashlib
import hmac
import json
import time
from typing import Any, Dict, Optional


def base64url_encode(data: bytes) -> str:
    return base64.urlsafe_b64encode(data).rstrip(b"=").decode("utf-8")


def base64url_decode(data: str) -> bytes:
    padding = "=" * ((4 - len(data) % 4) % 4)
    return base64.urlsafe_b64decode((data + padding).encode("utf-8"))


def generate_auth_token(
    user_id: str,
    secret_key: str,
    expires_in_seconds: int = 3600,
    extra_claims: Optional[Dict[str, Any]] = None,
) -> str:
    header = {"alg": "HS256", "typ": "JWT"}
    now = int(time.time())

    payload = {
        "sub": str(user_id),
        "iat": now,
        "exp": now + expires_in_seconds,
    }

    if extra_claims:
        payload.update(extra_claims)

    encoded_header = base64url_encode(
        json.dumps(header, separators=(",", ":")).encode("utf-8")
    )
    encoded_payload = base64url_encode(
        json.dumps(payload, separators=(",", ":")).encode("utf-8")
    )

    signing_input = f"{encoded_header}.{encoded_payload}".encode("utf-8")
    signature = hmac.new(
        secret_key.encode("utf-8"), signing_input, hashlib.sha256
    ).digest()
    encoded_signature = base64url_encode(signature)

    return f"{encoded_header}.{encoded_payload}.{encoded_signature}"


def verify_auth_token(token: str, secret_key: str) -> Optional[Dict[str, Any]]:
    parts = token.split(".")
    if len(parts) != 3:
        return None

    encoded_header, encoded_payload, encoded_signature = parts

    signing_input = f"{encoded_header}.{encoded_payload}".encode("utf-8")
    expected_sig = hmac.new(
        secret_key.encode("utf-8"), signing_input, hashlib.sha256
    ).digest()

    try:
        provided_sig = base64url_decode(encoded_signature)
    except Exception:
        return None

    if not hmac.compare_digest(expected_sig, provided_sig):
        return None

    try:
        payload_bytes = base64url_decode(encoded_payload)
        payload = json.loads(payload_bytes.decode("utf-8"))
    except Exception:
        return None

    exp = payload.get("exp")
    if exp is not None and time.time() > exp:
        return None

    return payload


if __name__ == "__main__":
    secret = "super-secret-key-12345"
    uid = "user_42"

    token = generate_auth_token(
        user_id=uid,
        secret_key=secret,
        expires_in_seconds=3600,
        extra_claims={"role": "admin", "email": "user42@example.com"},
    )
    print("Generated Token:\n", token)

    decoded_payload = verify_auth_token(token=token, secret_key=secret)
    print("\nVerified Payload:\n", decoded_payload)

    invalid_payload = verify_auth_token(token=token, secret_key="wrong-secret")
    print("\nVerification with Invalid Key:\n", invalid_payload)