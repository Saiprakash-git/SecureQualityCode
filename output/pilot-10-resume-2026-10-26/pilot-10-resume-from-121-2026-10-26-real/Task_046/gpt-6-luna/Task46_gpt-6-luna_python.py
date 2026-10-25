import hashlib
import hmac
import re
import secrets
import sqlite3
from contextlib import closing
from datetime import datetime, timezone


PASSWORD_ITERATIONS = 600_000
EMAIL_PATTERN = re.compile(r"^[^@\s]+@[^@\s]+\.[^@\s]+$")


def initialize_database(db_path):
    with closing(sqlite3.connect(db_path)) as connection:
        connection.execute(
            """
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                email TEXT NOT NULL UNIQUE COLLATE NOCASE,
                password_salt BLOB NOT NULL,
                password_hash BLOB NOT NULL,
                created_at TEXT NOT NULL
            )
            """
        )
        connection.commit()


def hash_password(password, salt):
    return hashlib.pbkdf2_hmac(
        "sha256",
        password.encode("utf-8"),
        salt,
        PASSWORD_ITERATIONS,
    )


def register_user(name, email, password, db_path):
    name = name.strip()
    email = email.strip().lower()

    if not name:
        return {"success": False, "message": "Name is required."}
    if len(name) > 200:
        return {"success": False, "message": "Name must be 200 characters or fewer."}
    if not EMAIL_PATTERN.fullmatch(email):
        return {"success": False, "message": "Enter a valid email address."}
    if len(password) < 8:
        return {"success": False, "message": "Password must be at least 8 characters."}

    salt = secrets.token_bytes(16)
    password_hash = hash_password(password, salt)
    created_at = datetime.now(timezone.utc).isoformat()

    initialize_database(db_path)

    try:
        with closing(sqlite3.connect(db_path)) as connection:
            connection.execute(
                """
                INSERT INTO users (name, email, password_salt, password_hash, created_at)
                VALUES (?, ?, ?, ?, ?)
                """,
                (name, email, salt, password_hash, created_at),
            )
            connection.commit()
    except sqlite3.IntegrityError:
        return {"success": False, "message": "An account with that email already exists."}
    except sqlite3.Error:
        return {"success": False, "message": "Unable to save the account."}

    return {"success": True, "message": "Registration successful."}


def verify_password(email, password, db_path):
    normalized_email = email.strip().lower()
    try:
        with closing(sqlite3.connect(db_path)) as connection:
            row = connection.execute(
                "SELECT password_salt, password_hash FROM users WHERE email = ?",
                (normalized_email,),
            ).fetchone()
    except sqlite3.Error:
        return False

    if row is None:
        return False

    salt, stored_hash = row
    candidate_hash = hash_password(password, salt)
    return hmac.compare_digest(candidate_hash, stored_hash)


def run_cli(input_fn, password_fn, output_fn, db_path):
    name = input_fn("Name: ")
    email = input_fn("Email: ")
    password = password_fn("Password: ")

    result = register_user(name, email, password, db_path)
    output_fn(result["message"])
    return result


if __name__ == "__main__":
    from getpass import getpass

    run_cli(input, getpass, print, "users.db")