import hashlib
import hmac
import re
import secrets
import sqlite3
from typing import Optional, Tuple


def init_db(conn: sqlite3.Connection) -> None:
    with conn:
        conn.execute(
            """
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE NOT NULL COLLATE NOCASE,
                password_hash BLOB NOT NULL,
                salt BLOB NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
            """
        )


def validate_password_strength(password: str) -> Tuple[bool, str]:
    if len(password) < 8:
        return False, "Password must be at least 8 characters long."
    if not re.search(r"[A-Z]", password):
        return False, "Password must contain at least one uppercase letter."
    if not re.search(r"[a-z]", password):
        return False, "Password must contain at least one lowercase letter."
    if not re.search(r"\d", password):
        return False, "Password must contain at least one digit."
    if not re.search(r"[!@#$%^&*(),.?\":{}|<>]", password):
        return False, "Password must contain at least one special character."
    return True, "Password meets complexity requirements."


def derive_key(password: str, salt: bytes) -> bytes:
    return hashlib.scrypt(
        password.encode("utf-8"),
        salt=salt,
        n=16384,
        r=8,
        p=1,
        dklen=64
    )


def register_user(conn: sqlite3.Connection, username: str, password: str) -> Tuple[bool, str]:
    cleaned_username = username.strip()
    if not cleaned_username:
        return False, "Username cannot be empty."

    is_strong, msg = validate_password_strength(password)
    if not is_strong:
        return False, msg

    salt = secrets.token_bytes(16)
    key = derive_key(password, salt)

    try:
        with conn:
            conn.execute(
                "INSERT INTO users (username, password_hash, salt) VALUES (?, ?, ?)",
                (cleaned_username, key, salt)
            )
        return True, "User registered successfully."
    except sqlite3.IntegrityError:
        return False, "Username already exists."


def authenticate_user(conn: sqlite3.Connection, username: str, password: str) -> Tuple[bool, str]:
    cleaned_username = username.strip()
    cursor = conn.cursor()
    cursor.execute(
        "SELECT password_hash, salt FROM users WHERE username = ?",
        (cleaned_username,)
    )
    row = cursor.fetchone()

    if row is None:
        dummy_salt = b"\x00" * 16
        derive_key(password, dummy_salt)
        return False, "Invalid username or password."

    stored_hash, salt = row
    computed_hash = derive_key(password, salt)

    if hmac.compare_digest(computed_hash, stored_hash):
        return True, "Authentication successful."
    else:
        return False, "Invalid username or password."


def change_password(
    conn: sqlite3.Connection,
    username: str,
    old_password: str,
    new_password: str
) -> Tuple[bool, str]:
    auth_success, _ = authenticate_user(conn, username, old_password)
    if not auth_success:
        return False, "Authentication failed. Current password is incorrect."

    is_strong, msg = validate_password_strength(new_password)
    if not is_strong:
        return False, msg

    new_salt = secrets.token_bytes(16)
    new_key = derive_key(new_password, new_salt)

    with conn:
        conn.execute(
            "UPDATE users SET password_hash = ?, salt = ? WHERE username = ?",
            (new_key, new_salt, username.strip())
        )

    return True, "Password updated successfully."


if __name__ == "__main__":
    db_connection = sqlite3.connect(":memory:")
    init_db(db_connection)

    reg_ok, reg_msg = register_user(db_connection, "Alice", "P@ssw0rd123!")
    print(f"Register Alice: {reg_msg}")

    auth_ok, auth_msg = authenticate_user(db_connection, "Alice", "P@ssw0rd123!")
    print(f"Auth Alice (valid): {auth_msg}")

    auth_bad, auth_bad_msg = authenticate_user(db_connection, "Alice", "WrongPass123!")
    print(f"Auth Alice (invalid): {auth_bad_msg}")

    auth_ghost, auth_ghost_msg = authenticate_user(db_connection, "Bob", "DummyPass123!")
    print(f"Auth Bob (nonexistent): {auth_ghost_msg}")

    chg_ok, chg_msg = change_password(db_connection, "Alice", "P@ssw0rd123!", "NewS3curePass#")
    print(f"Change Alice Password: {chg_msg}")

    auth_old, auth_old_msg = authenticate_user(db_connection, "Alice", "P@ssw0rd123!")
    print(f"Auth Alice with old pass: {auth_old_msg}")

    auth_new, auth_new_msg = authenticate_user(db_connection, "Alice", "NewS3curePass#")
    print(f"Auth Alice with new pass: {auth_new_msg}")

    db_connection.close()