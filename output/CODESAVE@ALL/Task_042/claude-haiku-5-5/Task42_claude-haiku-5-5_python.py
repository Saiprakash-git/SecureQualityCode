import hashlib
import hmac
import os
import re
import secrets
import sqlite3
import threading
import time


PASSWORD_MIN_LENGTH = 8
PASSWORD_MAX_LENGTH = 128
USERNAME_PATTERN = re.compile(r"^[A-Za-z0-9_.-]{3,32}$")
PBKDF2_ITERATIONS = 310000
SALT_BYTES = 16
MAX_FAILED_ATTEMPTS = 5
LOCKOUT_SECONDS = 15 * 60
SESSION_TTL_SECONDS = 60 * 60


def hash_password(password, iterations=PBKDF2_ITERATIONS):
    salt = secrets.token_bytes(SALT_BYTES)
    digest = hashlib.pbkdf2_hmac("sha256", password.encode("utf-8"), salt, iterations)
    return f"pbkdf2_sha256${iterations}${salt.hex()}${digest.hex()}"


def verify_password_hash(password, stored_hash):
    try:
        algorithm, iterations, salt_hex, digest_hex = stored_hash.split("$")
        if algorithm != "pbkdf2_sha256":
            return False
        iterations = int(iterations)
        salt = bytes.fromhex(salt_hex)
        expected = bytes.fromhex(digest_hex)
    except (ValueError, AttributeError):
        return False
    candidate = hashlib.pbkdf2_hmac("sha256", password.encode("utf-8"), salt, iterations)
    return hmac.compare_digest(candidate, expected)


def validate_username(username):
    if not isinstance(username, str) or not USERNAME_PATTERN.match(username):
        return False, "Username must be 3-32 characters: letters, digits, '_', '.', '-'."
    return True, ""


def validate_password(password):
    if not isinstance(password, str):
        return False, "Password must be a string."
    if len(password) < PASSWORD_MIN_LENGTH:
        return False, f"Password must be at least {PASSWORD_MIN_LENGTH} characters."
    if len(password) > PASSWORD_MAX_LENGTH:
        return False, f"Password must be at most {PASSWORD_MAX_LENGTH} characters."
    if not re.search(r"[A-Za-z]", password) or not re.search(r"\d", password):
        return False, "Password must contain at least one letter and one digit."
    return True, ""


class AuthSystem:
    def __init__(self, db_path=":memory:"):
        self._lock = threading.Lock()
        self._conn = sqlite3.connect(db_path, check_same_thread=False)
        self._conn.execute(
            """
            CREATE TABLE IF NOT EXISTS users (
                username TEXT PRIMARY KEY,
                password_hash TEXT NOT NULL,
                failed_attempts INTEGER NOT NULL DEFAULT 0,
                locked_until REAL NOT NULL DEFAULT 0
            )
            """
        )
        self._conn.commit()
        self._sessions = {}

    def register(self, username, password):
        ok, msg = validate_username(username)
        if not ok:
            return False, msg
        ok, msg = validate_password(password)
        if not ok:
            return False, msg
        password_hash = hash_password(password)
        with self._lock:
            try:
                self._conn.execute(
                    "INSERT INTO users (username, password_hash) VALUES (?, ?)",
                    (username, password_hash),
                )
                self._conn.commit()
            except sqlite3.IntegrityError:
                return False, "Username already exists."
        return True, "User registered successfully."

    def authenticate(self, username, password):
        if not isinstance(username, str) or not isinstance(password, str):
            return None
        now = time.time()
        with self._lock:
            row = self._conn.execute(
                "SELECT password_hash, failed_attempts, locked_until FROM users WHERE username = ?",
                (username,),
            ).fetchone()
            if row is None:
                # Perform a dummy hash to reduce timing-based user enumeration.
                hash_password(password)
                return None
            stored_hash, failed, locked_until = row
            if locked_until > now:
                return None
            if verify_password_hash(password, stored_hash):
                self._conn.execute(
                    "UPDATE users SET failed_attempts = 0, locked_until = 0 WHERE username = ?",
                    (username,),
                )
                self._conn.commit()
                return self._create_session(username)
            failed += 1
            if failed >= MAX_FAILED_ATTEMPTS:
                self._conn.execute(
                    "UPDATE users SET failed_attempts = 0, locked_until = ? WHERE username = ?",
                    (now + LOCKOUT_SECONDS, username),
                )
            else:
                self._conn.execute(
                    "UPDATE users SET failed_attempts = ? WHERE username = ?",
                    (failed, username),
                )
            self._conn.commit()
            return None

    def change_password(self, username, old_password, new_password):
        ok, msg = validate_password(new_password)
        if not ok:
            return False, msg
        if self.authenticate(username, old_password) is None:
            return False, "Current credentials are invalid."
        with self._lock:
            self._conn.execute(
                "UPDATE users SET password_hash = ? WHERE username = ?",
                (hash_password(new_password), username),
            )
            self._conn.commit()
        self.logout_all(username)
        return True, "Password changed successfully."

    def validate_session(self, token):
        if not isinstance(token, str):
            return None
        with self._lock:
            session = self._sessions.get(token)
            if session is None:
                return None
            username, expires_at = session
            if time.time() > expires_at:
                del self._sessions[token]
                return None
            return username

    def logout(self, token):
        with self._lock:
            return self._sessions.pop(token, None) is not None

    def logout_all(self, username):
        with self._lock:
            tokens = [t for t, (u, _) in self._sessions.items() if u == username]
            for t in tokens:
                del self._sessions[t]
            return len(tokens)

    def _create_session(self, username):
        token = secrets.token_urlsafe(32)
        self._sessions[token] = (username, time.time() + SESSION_TTL_SECONDS)
        return token

    def close(self):
        with self._lock:
            self._conn.close()


if __name__ == "__main__":
    auth = AuthSystem()
    print(auth.register("alice", "SecurePass123"))
    token = auth.authenticate("alice", "SecurePass123")
    print("Session token issued:", token is not None)
    print("Session user:", auth.validate_session(token))
    print("Logout:", auth.logout(token))
    auth.close()