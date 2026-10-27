import sys
import sqlite3


def get_customer_by_username(db_path, customerusername):
    conn = sqlite3.connect(db_path)
    try:
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()
        cursor.execute(
            "SELECT * FROM customer WHERE customerusername = ?",
            (customerusername,)
        )
        row = cursor.fetchone()
        if row is None:
            return None
        return dict(row)
    finally:
        conn.close()


def main():
    if len(sys.argv) < 2:
        print("Usage: python script.py <customerusername> [db_path]")
        sys.exit(1)

    customerusername = sys.argv[1]
    db_path = sys.argv[2] if len(sys.argv) > 2 else "database.db"

    customer = get_customer_by_username(db_path, customerusername)
    if customer is None:
        print(f"No customer found with username: {customerusername}")
        sys.exit(1)

    for key, value in customer.items():
        print(f"{key}: {value}")
    return customer


if __name__ == "__main__":
    main()