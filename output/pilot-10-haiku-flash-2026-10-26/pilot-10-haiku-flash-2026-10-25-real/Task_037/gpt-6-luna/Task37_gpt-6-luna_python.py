import sqlite3
from typing import Any, Optional


def get_user_info(username: str, database_path: str = "users.db") -> Optional[dict[str, Any]]:
    """Return the user record matching username, or None if no match exists."""
    with sqlite3.connect(database_path) as connection:
        connection.row_factory = sqlite3.Row
        row = connection.execute(
            "SELECT * FROM users WHERE username = ?",
            (username,),
        ).fetchone()
        return dict(row) if row is not None else None


def main() -> None:
    username = input("Enter a username: ").strip()
    user = get_user_info(username)

    if user is None:
        print("User not found.")
    else:
        for key, value in user.items():
            print(f"{key}: {value}")


if __name__ == "__main__":
    main()