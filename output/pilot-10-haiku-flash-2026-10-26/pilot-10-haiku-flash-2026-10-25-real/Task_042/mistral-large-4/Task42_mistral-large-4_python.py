import hashlib
import secrets
import hmac
import re
from typing import Dict, Optional, Tuple

ITERATIONS = 100000
SALT_SIZE = 32
TOKEN_SIZE = 32

def _validate_username(username: str) -> bool:
    if not isinstance(username, str):
        return False
    if len(username) < 3 or len(username) > 32:
        return False
    return bool(re.match(r'^[a-zA-Z0-9_]+$', username))

def _validate_password(password: str) -> bool:
    if not isinstance(password, str):
        return False
    return len(password) >= 8

def hash_password(password: str) -> str:
    salt = secrets.token_bytes(SALT_SIZE)
    dk = hashlib.pbkdf2_hmac('sha256', password.encode('utf-8'), salt, ITERATIONS)
    return f"{salt.hex()}:{dk.hex()}"

def verify_password(password: str, stored_hash: str) -> bool:
    try:
        salt_hex, hash_hex = stored_hash.split(':')
        salt = bytes.fromhex(salt_hex)
        expected_hash = bytes.fromhex(hash_hex)
        dk = hashlib.pbkdf2_hmac('sha256', password.encode('utf-8'), salt, ITERATIONS)
        return hmac.compare_digest(dk, expected_hash)
    except (ValueError, AttributeError):
        return False

def register_user(users: Dict[str, str], username: str, password: str) -> Tuple[bool, str]:
    if not _validate_username(username):
        return False, "Invalid username. Must be 3-32 characters, alphanumeric and underscore only."
    if not _validate_password(password):
        return False, "Invalid password. Must be at least 8 characters."
    if username in users:
        return False, "Username already exists."

    users[username] = hash_password(password)
    return True, "User registered successfully."

def authenticate_user(users: Dict[str, str], sessions: Dict[str, str], username: str, password: str) -> Tuple[bool, Optional[str]]:
    if username not in users:
        return False, None

    if not verify_password(password, users[username]):
        return False, None

    token = secrets.token_urlsafe(TOKEN_SIZE)
    sessions[token] = username
    return True, token

def validate_token(sessions: Dict[str, str], token: str) -> Optional[str]:
    return sessions.get(token)

def logout_user(sessions: Dict[str, str], token: str) -> bool:
    if token in sessions:
        del sessions[token]
        return True
    return False

def change_password(users: Dict[str, str], username: str, old_password: str, new_password: str) -> Tuple[bool, str]:
    if username not in users:
        return False, "User not found."
    if not _validate_password(new_password):
        return False, "Invalid new password. Must be at least 8 characters."
    if not verify_password(old_password, users[username]):
        return False, "Incorrect old password."

    users[username] = hash_password(new_password)
    return True, "Password changed successfully."