import sqlite3
from typing import Any, Dict, Optional


def create_database(db_path: str) -> None:
    with sqlite3.connect(db_path) as conn:
        cursor = conn.cursor()
        cursor.execute(
            """
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE NOT NULL,
                email TEXT NOT NULL,
                full_name TEXT NOT NULL,
                role TEXT NOT NULL
            )
            """
        )
        sample_users = [
            ("alice", "alice@example.com", "Alice Smith", "Admin"),
            ("bob", "bob@example.com", "Bob Jones", "User"),
            ("charlie", "charlie@example.com", "Charlie Brown", "User"),
        ]
        cursor.executemany(
            """
            INSERT OR IGNORE INTO users (username, email, full_name, role)
            VALUES (?, ?, ?, ?)
            """,
            sample_users,
        )
        conn.commit()


def get_user_by_username(db_path: str, username: str) -> Optional[Dict[str, Any]]:
    with sqlite3.connect(db_path) as conn:
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()
        cursor.execute(
            """
            SELECT id, username, email, full_name, role
            FROM users
            WHERE username = ?
            """,
            (username,),
        )
        row = cursor.fetchone()
        return dict(row) if row else None


def main() -> None:
    db_path = "users.db"
    create_database(db_path)

    try:
        username_query = input("Enter username to retrieve: ").strip()
    except (KeyboardInterrupt, EOFError):
        return

    if not username_query:
        print("Error: Username cannot be empty.")
        return

    user = get_user_by_username(db_path, username_query)

    if user:
        print("User Record Found:")
        for key, value in user.items():
            print(f"  {key}: {value}")
    else:
        print(f"No user found with username: '{username_query}'")


if __name__ == "__main__":
    main()