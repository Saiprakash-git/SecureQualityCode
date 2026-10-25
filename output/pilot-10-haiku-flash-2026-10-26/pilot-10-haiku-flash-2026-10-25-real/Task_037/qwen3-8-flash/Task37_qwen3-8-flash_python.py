import json
import sqlite3
import sys

DEFAULT_DB_PATH = "users.db"

def initialize_database(db_path):
    conn = sqlite3.connect(db_path)
    try:
        cursor = conn.cursor()
        cursor.execute(
            """
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE NOT NULL,
                email TEXT NOT NULL,
                full_name TEXT
            )
            """
        )

        cursor.execute("SELECT COUNT(*) FROM users")
        if cursor.fetchone()[0] == 0:
            sample_users = [
                ("alice", "alice@example.com", "Alice Johnson"),
                ("bob", "bob@example.com", "Bob Smith"),
                ("charlie", "charlie@example.com", "Charlie Brown"),
            ]
            cursor.executemany(
                "INSERT INTO users (username, email, full_name) VALUES (?, ?, ?)",
                sample_users,
            )

        conn.commit()
    finally:
        conn.close()

def get_user_information(username, db_path=DEFAULT_DB_PATH):
    initialize_database(db_path)

    conn = sqlite3.connect(db_path)
    try:
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()
        cursor.execute(
            "SELECT id, username, email, full_name FROM users WHERE username = ?",
            (username,),
        )

        row = cursor.fetchone()
        if row is None:
            return None

        return {
            "id": row["id"],
            "username": row["username"],
            "email": row["email"],
            "full_name": row["full_name"],
        }
    finally:
        conn.close()

def main(argv=None):
    if argv is None:
        argv = sys.argv[1:]

    if argv:
        username = argv[0]
    else:
        username = input("Enter username: ")

    try:
        user = get_user_information(username)
    except sqlite3.Error as exc:
        print("Database error:", exc)
        return 1

    if user is None:
        print("User not found.")
    else:
        print(json.dumps(user, indent=2))

    return 0

if __name__ == "__main__":
    sys.exit(main())