import os
import re
import sqlite3
import hashlib

def init_db(db_path: str = "users.db") -> None:
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            email TEXT NOT NULL UNIQUE,
            password_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    """)
    conn.commit()
    conn.close()

def is_valid_email(email: str) -> bool:
    email_regex = r"^[\w\.-]+@[\w\.-]+\.\w+$"
    return bool(re.match(email_regex, email))

def is_valid_password(password: str) -> bool:
    return len(password) >= 8

def hash_password(password: str, salt: bytes = None) -> tuple:
    if salt is None:
        salt = os.urandom(16)
    hashed = hashlib.pbkdf2_hmac(
        "sha256",
        password.encode("utf-8"),
        salt,
        iterations=100000
    )
    return hashed.hex(), salt.hex()

def register_user(db_path: str, name: str, email: str, password: str) -> dict:
    name = name.strip()
    email = email.strip().lower()

    if not name:
        return {"success": False, "message": "Name cannot be empty."}

    if not is_valid_email(email):
        return {"success": False, "message": "Invalid email address format."}

    if not is_valid_password(password):
        return {"success": False, "message": "Password must be at least 8 characters long."}

    pwd_hash, salt = hash_password(password)

    try:
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()
        cursor.execute(
            "INSERT INTO users (name, email, password_hash, salt) VALUES (?, ?, ?, ?)",
            (name, email, pwd_hash, salt)
        )
        conn.commit()
        user_id = cursor.lastrowid
        conn.close()
        return {
            "success": True,
            "message": "User registered successfully.",
            "user_id": user_id
        }
    except sqlite3.IntegrityError:
        return {"success": False, "message": "Email is already registered."}
    except sqlite3.Error as e:
        return {"success": False, "message": f"Database error: {str(e)}"}

def get_user_by_email(db_path: str, email: str) -> dict:
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    cursor.execute(
        "SELECT id, name, email, created_at FROM users WHERE email = ?",
        (email.strip().lower(),)
    )
    row = cursor.fetchone()
    conn.close()
    if row:
        return {
            "id": row[0],
            "name": row[1],
            "email": row[2],
            "created_at": row[3]
        }
    return None

def main() -> None:
    db_file = "users.db"
    init_db(db_file)

    print("=== User Registration System ===")
    name = input("Enter your full name: ")
    email = input("Enter your email address: ")
    password = input("Enter your password (min 8 chars): ")

    result = register_user(db_file, name, email, password)

    if result["success"]:
        print(f"Success: {result['message']} (User ID: {result['user_id']})")
        user = get_user_by_email(db_file, email)
        print("Stored User Details:", user)
    else:
        print(f"Error: {result['message']}")

if __name__ == "__main__":
    main()