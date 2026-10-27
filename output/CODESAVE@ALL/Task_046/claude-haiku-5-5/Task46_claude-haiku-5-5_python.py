import re
import sqlite3
import hashlib
import secrets
import sys

EMAIL_PATTERN = re.compile(r"^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$")


def init_db(db_path=":memory:"):
    conn = sqlite3.connect(db_path)
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
    return conn


def validate_name(name):
    if not isinstance(name, str) or not name.strip():
        return "Name is required."
    if len(name.strip()) > 100:
        return "Name must be at most 100 characters."
    return None


def validate_email(email):
    if not isinstance(email, str) or not email.strip():
        return "Email is required."
    if not EMAIL_PATTERN.match(email.strip()):
        return "Email format is invalid."
    return None


def validate_password(password):
    if not isinstance(password, str) or not password:
        return "Password is required."
    if len(password) < 8:
        return "Password must be at least 8 characters."
    if not re.search(r"[A-Za-z]", password) or not re.search(r"\d", password):
        return "Password must contain at least one letter and one digit."
    return None


def hash_password(password, salt=None):
    if salt is None:
        salt = secrets.token_hex(16)
    digest = hashlib.pbkdf2_hmac("sha256", password.encode("utf-8"), salt.encode("utf-8"), 100000)
    return digest.hex(), salt


def register_user(conn, name, email, password):
    errors = [e for e in (validate_name(name), validate_email(email), validate_password(password)) if e]
    if errors:
        return {"success": False, "errors": errors}

    email = email.strip().lower()
    name = name.strip()
    password_hash, salt = hash_password(password)

    try:
        with conn:
            conn.execute(
                "INSERT INTO users (name, email, password_hash, salt) VALUES (?, ?, ?, ?)",
                (name, email, password_hash, salt),
            )
    except sqlite3.IntegrityError:
        return {"success": False, "errors": ["Email is already registered."]}

    return {"success": True, "errors": []}


def get_user_by_email(conn, email):
    row = conn.execute(
        "SELECT id, name, email, created_at FROM users WHERE email = ?",
        (email.strip().lower(),),
    ).fetchone()
    if row is None:
        return None
    return {"id": row[0], "name": row[1], "email": row[2], "created_at": row[3]}


def verify_password(conn, email, password):
    row = conn.execute(
        "SELECT password_hash, salt FROM users WHERE email = ?",
        (email.strip().lower(),),
    ).fetchone()
    if row is None:
        return False
    computed, _ = hash_password(password, row[1])
    return secrets.compare_digest(computed, row[0])


def prompt_registration(input_func=input):
    name = input_func("Name: ")
    email = input_func("Email: ")
    password = input_func("Password: ")
    return name, email, password


def main():
    db_path = sys.argv[1] if len(sys.argv) > 1 else "users.db"
    conn = init_db(db_path)
    try:
        name, email, password = prompt_registration()
        result = register_user(conn, name, email, password)
        if result["success"]:
            print("Registration successful.")
        else:
            print("Registration failed:")
            for error in result["errors"]:
                print(f" - {error}")
    finally:
        conn.close()


if __name__ == "__main__":
    main()