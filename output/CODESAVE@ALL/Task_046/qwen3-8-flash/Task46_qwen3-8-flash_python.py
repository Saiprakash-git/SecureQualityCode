import sqlite3
import hashlib
import os
import re
import hmac

DEFAULT_DB_PATH = "users.db"


def initialize_database(db_path=DEFAULT_DB_PATH):
    with sqlite3.connect(db_path) as conn:
        conn.execute(
            """
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                email TEXT NOT NULL UNIQUE,
                password_hash TEXT NOT NULL,
                salt TEXT NOT NULL,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
            """
        )
        conn.commit()
    return True


def validate_email(email):
    if not email:
        return False
    pattern = r"^[^@\s]+@[^@\s]+\.[^@\s]+$"
    return bool(re.fullmatch(pattern, email.strip()))


def hash_password(password, salt_bytes=None):
    if salt_bytes is None:
        salt_bytes = os.urandom(16)

    salt_hex = salt_bytes.hex()
    password_hash = hashlib.pbkdf2_hmac(
        "sha256",
        password.encode("utf-8"),
        salt_bytes,
        100000
    ).hex()

    return password_hash, salt_hex


def verify_password(password, stored_hash, salt_hex):
    try:
        salt_bytes = bytes.fromhex(salt_hex)
    except ValueError:
        return False

    password_hash = hashlib.pbkdf2_hmac(
        "sha256",
        password.encode("utf-8"),
        salt_bytes,
        100000
    ).hex()

    return hmac.compare_digest(password_hash, stored_hash)


def register_user(name, email, password, db_path=DEFAULT_DB_PATH):
    name = (name or "").strip()
    email = (email or "").strip().lower()
    password = password or ""

    errors = []

    if not name:
        errors.append("Name is required.")

    if not validate_email(email):
        errors.append("Valid email is required.")

    if len(password) < 8:
        errors.append("Password must be at least 8 characters long.")

    if errors:
        return {
            "success": False,
            "errors": errors
        }

    initialize_database(db_path)

    password_hash, salt_hex = hash_password(password)

    conn = sqlite3.connect(db_path)
    try:
        cursor = conn.execute(
            """
            INSERT INTO users (name, email, password_hash, salt)
            VALUES (?, ?, ?, ?)
            """,
            (name, email, password_hash, salt_hex)
        )
        conn.commit()

        return {
            "success": True,
            "user_id": cursor.lastrowid,
            "name": name,
            "email": email
        }
    except sqlite3.IntegrityError:
        return {
            "success": False,
            "errors": ["Email already registered."]
        }
    finally:
        conn.close()


def get_user_by_email(email, db_path=DEFAULT_DB_PATH):
    email = (email or "").strip().lower()

    conn = sqlite3.connect(db_path)
    conn.row_factory = sqlite3.Row

    try:
        cursor = conn.execute(
            """
            SELECT id, name, email, password_hash, salt, created_at
            FROM users
            WHERE email = ?
            """,
            (email,)
        )
        row = cursor.fetchone()

        if row is None:
            return None

        return dict(row)
    finally:
        conn.close()


def authenticate_user(email, password, db_path=DEFAULT_DB_PATH):
    user = get_user_by_email(email, db_path)

    if user is None:
        return {
            "success": False,
            "error": "Invalid email or password."
        }

    if verify_password(password, user["password_hash"], user["salt"]):
        return {
            "success": True,
            "user": {
                "id": user["id"],
                "name": user["name"],
                "email": user["email"]
            }
        }

    return {
        "success": False,
        "error": "Invalid email or password."
    }


def main():
    initialize_database()

    print("User Registration System")

    name = input("Name: ").strip()
    email = input("Email: ").strip()
    password = input("Password: ").strip()

    result = register_user(name, email, password)

    if result["success"]:
        print("Registration successful.")
        print(f"User ID: {result['user_id']}")
    else:
        print("Registration failed.")
        for error in result.get("errors", []):
            print(f"- {error}")


if __name__ == "__main__":
    main()