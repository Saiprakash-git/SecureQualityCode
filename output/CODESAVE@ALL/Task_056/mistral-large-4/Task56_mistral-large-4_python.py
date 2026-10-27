import secrets
import hashlib
import hmac
import base64
import json
import time

def generate_random_token(length=32):
    return secrets.token_hex(length)

def generate_url_safe_token(length=32):
    return secrets.token_urlsafe(length)

def generate_api_key(prefix="sk", length=32):
    return f"{prefix}_{secrets.token_hex(length)}"

def generate_signed_token(payload, secret_key, expires_in=3600):
    if isinstance(payload, dict):
        payload = json.dumps(payload, separators=(',', ':'))
    elif not isinstance(payload, str):
        payload = str(payload)

    expiration = int(time.time()) + expires_in
    message = f"{payload}.{expiration}"

    signature = hmac.new(
        secret_key.encode('utf-8'),
        message.encode('utf-8'),
        hashlib.sha256
    ).hexdigest()

    encoded_message = base64.urlsafe_b64encode(message.encode('utf-8')).decode('utf-8')
    return f"{encoded_message}.{signature}"

def verify_token(token, secret_key):
    try:
        encoded_message, signature = token.rsplit('.', 1)
        message = base64.urlsafe_b64decode(encoded_message.encode('utf-8')).decode('utf-8')

        expected_signature = hmac.new(
            secret_key.encode('utf-8'),
            message.encode('utf-8'),
            hashlib.sha256
        ).hexdigest()

        if not hmac.compare_digest(signature, expected_signature):
            return None

        payload_str, expiration_str = message.rsplit('.', 1)
        expiration = int(expiration_str)

        if time.time() > expiration:
            return None

        try:
            payload = json.loads(payload_str)
        except json.JSONDecodeError:
            payload = payload_str

        return payload
    except Exception:
        return None

def generate_session_token(user_id, secret_key, expires_in=86400):
    payload = {
        "user_id": user_id,
        "iat": int(time.time()),
        "type": "session"
    }
    return generate_signed_token(payload, secret_key, expires_in)

def hash_token(token, salt=None):
    if salt is None:
        salt = secrets.token_hex(16)
    hashed = hashlib.pbkdf2_hmac('sha256', token.encode('utf-8'), salt.encode('utf-8'), 100000)
    return f"{salt}${hashed.hex()}"

def verify_hashed_token(token, stored_hash):
    try:
        salt, expected_hash = stored_hash.split('$', 1)
        hashed = hashlib.pbkdf2_hmac('sha256', token.encode('utf-8'), salt.encode('utf-8'), 100000)
        return hmac.compare_digest(hashed.hex(), expected_hash)
    except Exception:
        return False