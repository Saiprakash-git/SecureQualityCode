import getpass
import sqlite3
import sys


def create_connection(db_path):
    return sqlite3.connect(db_path)


def setup_database(conn):
    cursor = conn.cursor()
    cursor.execute(
        """
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            email TEXT NOT NULL,
            full_name TEXT,
            created_at TEXT DEFAULT CURRENT_TIMESTAMP
        )
        """
    )
    cursor.execute("SELECT COUNT(*) FROM users")
    if cursor.fetchone()[0] == 0:
        sample_users = [
            ("alice", "alice@example.com", "Alice Smith"),
            ("bob", "bob@example.com", "Bob Johnson"),
            ("carol", "carol@example.com", "Carol Williams"),
        ]
        cursor.executemany(
            "INSERT INTO users (username, email, full_name) VALUES (?, ?, ?)",
            sample_users,
        )
    conn.commit()


def get_user_by_username(conn, username):
    cursor = conn.cursor()
    cursor.execute(
        "SELECT id, username, email, full_name, created_at FROM users WHERE username = ?",
        (username,),
    )
    row = cursor.fetchone()
    if row is None:
        return None
    columns = ["id", "username", "email", "full_name", "created_at"]
    return dict(zip(columns, row))


def format_user(user):
    if user is None:
        return "No user found."
    lines = [f"{key}: {value}" for key, value in user.items()]
    return "\n".join(lines)


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv
    db_path = argv[0] if argv else "users.db"

    conn = create_connection(db_path)
    try:
        setup_database(conn)
        while True:
            username = input("Enter username (or 'q' to quit): ").strip()
            if username.lower() == "q":
                break
            if not username:
                print("Username cannot be empty.")
                continue
            try:
                user = get_user_by_username(conn, username)
                print(format_user(user))
            except sqlite3.Error as e:
                print(f"Database error: {e}")
    except KeyboardInterrupt:
        print()
    finally:
        conn.close()


if __name__ == "__main__":
    main()