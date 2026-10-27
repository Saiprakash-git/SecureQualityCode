import hashlib
import hmac
import secrets
import threading
import unicodedata


class UserAuth:
    """In-memory authentication using salted PBKDF2-HMAC-SHA256 password hashes."""

    _ITERATIONS = 600_000
    _SALT_BYTES = 16
    _HASH_BYTES = 32
    _MAX_PASSWORD_BYTES = 1024
    _MIN_PASSWORD_LENGTH = 12

    def __init__(self):
        self._users = {}
        self._lock = threading.RLock()
        self._dummy_salt = secrets.token_bytes(self._SALT_BYTES)
        self._dummy_hash = self._derive(b"dummy-password", self._dummy_salt)

    @classmethod
    def _derive(cls, password_bytes, salt):
        return hashlib.pbkdf2_hmac(
            "sha256",
            password_bytes,
            salt,
            cls._ITERATIONS,
            dklen=cls._HASH_BYTES,
        )

    @staticmethod
    def _normalize_username(username):
        if not isinstance(username, str):
            return None

        normalized = unicodedata.normalize("NFKC", username).strip().casefold()
        if not 1 <= len(normalized) <= 64:
            return None
        if any(unicodedata.category(char).startswith("C") for char in normalized):
            return None

        return normalized

    @classmethod
    def _password_bytes(cls, password):
        if not isinstance(password, str):
            return None
        try:
            encoded = password.encode("utf-8")
        except UnicodeEncodeError:
            return None
        if len(encoded) > cls._MAX_PASSWORD_BYTES:
            return None
        return encoded

    @classmethod
    def _valid_new_password(cls, password, encoded):
        return (
            encoded is not None
            and len(password) >= cls._MIN_PASSWORD_LENGTH
        )

    def register(self, username, password):
        """Register an account; return False for invalid input or a duplicate name."""
        key = self._normalize_username(username)
        password_bytes = self._password_bytes(password)

        if key is None or not self._valid_new_password(password, password_bytes):
            return False

        salt = secrets.token_bytes(self._SALT_BYTES)
        password_hash = self._derive(password_bytes, salt)

        with self._lock:
            if key in self._users:
                return False
            self._users[key] = (salt, password_hash)

        return True

    def authenticate(self, username, password):
        """Return whether the supplied credentials are valid."""
        key = self._normalize_username(username)
        password_bytes = self._password_bytes(password)
        usable_password = password_bytes if password_bytes is not None else b""

        with self._lock:
            record = self._users.get(key) if key is not None else None

        if record is None:
            candidate = self._derive(usable_password, self._dummy_salt)
            hmac.compare_digest(candidate, self._dummy_hash)
            return False

        salt, expected_hash = record
        candidate = self._derive(usable_password, salt)
        return password_bytes is not None and hmac.compare_digest(
            candidate, expected_hash
        )

    def change_password(self, username, current_password, new_password):
        """Change a password after verifying the current password."""
        key = self._normalize_username(username)
        current_bytes = self._password_bytes(current_password)
        new_bytes = self._password_bytes(new_password)

        if key is None or not self._valid_new_password(new_password, new_bytes):
            return False

        with self._lock:
            record = self._users.get(key)
            if record is None:
                self._derive(
                    current_bytes if current_bytes is not None else b"",
                    self._dummy_salt,
                )
                return False

            salt, expected_hash = record
            candidate = self._derive(
                current_bytes if current_bytes is not None else b"",
                salt,
            )
            if current_bytes is None or not hmac.compare_digest(
                candidate, expected_hash
            ):
                return False

            new_salt = secrets.token_bytes(self._SALT_BYTES)
            self._users[key] = (new_salt, self._derive(new_bytes, new_salt))

        return True

    def delete_user(self, username, password):
        """Delete an account after verifying its password."""
        key = self._normalize_username(username)
        password_bytes = self._password_bytes(password)

        with self._lock:
            record = self._users.get(key) if key is not None else None
            if record is None:
                self._derive(
                    password_bytes if password_bytes is not None else b"",
                    self._dummy_salt,
                )
                return False

            salt, expected_hash = record
            candidate = self._derive(
                password_bytes if password_bytes is not None else b"",
                salt,
            )
            if password_bytes is None or not hmac.compare_digest(
                candidate, expected_hash
            ):
                return False

            del self._users[key]
            return True

    def user_count(self):
        """Return the number of registered accounts."""
        with self._lock:
            return len(self._users)