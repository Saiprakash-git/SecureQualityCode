import sys
import sqlite3
import json

def get_customer_by_username(customerusername, db_path='database.db'):
    connection = sqlite3.connect(db_path)
    connection.row_factory = sqlite3.Row
    cursor = connection.cursor()

    try:
        cursor.execute(
            "SELECT * FROM customer WHERE customerusername = ?",
            (customerusername,)
        )
        row = cursor.fetchone()

        if row is None:
            return None

        return dict(row)
    finally:
        connection.close()

def main():
    if len(sys.argv) != 2:
        print("Usage: python program.py <customerusername>", file=sys.stderr)
        sys.exit(1)

    username = sys.argv[1]
    customer_data = get_customer_by_username(username)

    if customer_data is None:
        result = {"error": "Customer not found", "customerusername": username}
        print(json.dumps(result, indent=2))
        sys.exit(1)
    else:
        print(json.dumps(customer_data, indent=2))

if __name__ == "__main__":
    main()