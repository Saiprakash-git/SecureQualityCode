import sqlite3
import hashlib
import re
import os
import getpass
from datetime import datetime

def init_db(db_path='users.db'):
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL,
            email TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            salt TEXT NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    ''')
    conn.commit()
    conn.close()
    return True

def validate_email(email):
    pattern = r'^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$'
    return re.match(pattern, email) is not None

def validate_password(password):
    if len(password) < 8:
        return False, "Password must be at least 8 characters long"
    if not re.search(r'[A-Z]', password):
        return False, "Password must contain at least one uppercase letter"
    if not re.search(r'[a-z]', password):
        return False, "Password must contain at least one lowercase letter"
    if not re.search(r'[0-9]', password):
        return False, "Password must contain at least one digit"
    return True, "Password is valid"

def hash_password(password, salt=None):
    if salt is None:
        salt = os.urandom(32).hex()
    password_hash = hashlib.pbkdf2_hmac('sha256', password.encode('utf-8'), bytes.fromhex(salt), 100000)
    return password_hash.hex(), salt

def register_user(name, email, password, db_path='users.db'):
    if not name or not name.strip():
        return False, "Name cannot be empty"

    if not validate_email(email):
        return False, "Invalid email format"

    is_valid, message = validate_password(password)
    if not is_valid:
        return False, message

    password_hash, salt = hash_password(password)

    try:
        conn = sqlite3.connect(db_path)
        cursor = conn.cursor()

        cursor.execute('SELECT id FROM users WHERE email = ?', (email.lower().strip(),))
        if cursor.fetchone():
            conn.close()
            return False, "Email already registered"

        cursor.execute(
            'INSERT INTO users (name, email, password_hash, salt) VALUES (?, ?, ?, ?)',
            (name.strip(), email.lower().strip(), password_hash, salt)
        )
        conn.commit()
        conn.close()
        return True, "User registered successfully"
    except sqlite3.Error as e:
        return False, f"Database error: {str(e)}"

def get_user_by_email(email, db_path='users.db'):
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    cursor.execute('SELECT id, name, email, created_at FROM users WHERE email = ?', (email.lower().strip(),))
    user = cursor.fetchone()
    conn.close()
    return user

def verify_password(email, password, db_path='users.db'):
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    cursor.execute('SELECT password_hash, salt FROM users WHERE email = ?', (email.lower().strip(),))
    result = cursor.fetchone()
    conn.close()

    if not result:
        return False

    stored_hash, salt = result
    input_hash, _ = hash_password(password, salt)
    return input_hash == stored_hash

def main():
    init_db()

    print("=== User Registration System ===")

    name = input("Enter your name: ").strip()
    email = input("Enter your email: ").strip()
    password = getpass.getpass("Enter your password: ")
    confirm_password = getpass.getpass("Confirm your password: ")

    if password != confirm_password:
        print("\nRegistration failed: Passwords do not match")
        return

    success, message = register_user(name, email, password)

    if success:
        print(f"\n{message}")
        print(f"Welcome, {name}!")
    else:
        print(f"\nRegistration failed: {message}")

if __name__ == "__main__":
    main()