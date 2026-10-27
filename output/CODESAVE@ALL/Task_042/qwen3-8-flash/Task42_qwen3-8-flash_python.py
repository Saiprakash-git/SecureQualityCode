import hashlib
import hmac
import secrets

PBKDF2_ITERATIONS = 200_000
SALT_LENGTH = 16
HASH_LENGTH = 32

_users = {}


def _derive_password_hash(password, salt=None):
    if not isinstance(password, str):
        raise TypeError("password must be a string")

    if salt is None:
        salt = secrets.token_bytes(SALT_LENGTH)

    password_hash = hashlib.pbkdf2_hmac(
        "sha256",
        password.encode("utf-8"),
        salt,
        PBKDF2_ITERATIONS,
        dklen=HASH_LENGTH,
    )

    return salt, password_hash


def _verify_password(password, stored_salt, stored_hash):
    _, computed_hash = _derive_password_hash(password, stored_salt)
    return hmac.compare_digest(computed_hash, stored_hash)


def _normalize_username(username):
    if not isinstance(username, str):
        raise TypeError("username must be a string")

    normalized = username.strip().lower()

    if not normalized:
        raise ValueError("username must not be empty")

    return normalized


def create_user(username, password):
    normalized_username = _normalize_username(username)

    if not isinstance(password, str) or len(password) < 8:
        return False

    if normalized_username in _users:
        return False

    salt, password_hash = _derive_password_hash(password)
    _users[normalized_username] = (salt, password_hash)

    return True


def authenticate(username, password):
    try:
        normalized_username = _normalize_username(username)
    except (TypeError, ValueError):
        return False

    if not isinstance(password, str):
        return False

    record = _users.get(normalized_username)

    if record is None:
        dummy_salt = b"\x00" * SALT_LENGTH
        dummy_hash = b"\x00" * HASH_LENGTH
        _verify_password(password, dummy_salt, dummy_hash)
        return False

    salt, stored_hash = record
    return _verify_password(password, salt, stored_hash)


def change_password(username, old_password, new_password):
    try:
        normalized_username = _normalize_username(username)
    except (TypeError, ValueError):
        return False

    if not isinstance(old_password, str) or not isinstance(new_password, str):
        return False

    if len(new_password) < 8:
        return False

    record = _users.get(normalized_username)

    if record is None:
        return False

    salt, stored_hash = record

    if not _verify_password(old_password, salt, stored_hash):
        return False

    new_salt, new_hash = _derive_password_hash(new_password)
    _users[normalized_username] = (new_salt, new_hash)

    return True


def delete_user(username):
    try:
        normalized_username = _normalize_username(username)
    except (TypeError, ValueError):
        return False

    if normalized_username not in _users:
        return False

    del _users[normalized_username]
    return True


def user_exists(username):
    try:
        normalized_username = _normalize_username(username)
    except (TypeError, ValueError):
        return False

    return normalized_username in _users