import argparse
import json
import sqlite3
import sys
from typing import Any, Dict, Optional


def get_customer_info(customer_username: str, db_path: str = "customers.db") -> Optional[Dict[str, Any]]:
    connection = sqlite3.connect(db_path)
    connection.row_factory = sqlite3.Row

    try:
        cursor = connection.cursor()
        cursor.execute(
            "SELECT * FROM customer WHERE customerusername = ?",
            (customer_username,)
        )
        row = cursor.fetchone()

        if row is None:
            return None

        return dict(row)
    finally:
        connection.close()


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Retrieve customer information from the customer table."
    )
    parser.add_argument(
        "customerusername",
        help="The customer username to look up"
    )
    parser.add_argument(
        "--db",
        default="customers.db",
        help="Path to the SQLite database file"
    )

    args = parser.parse_args()

    try:
        customer = get_customer_info(args.customerusername, args.db)
    except sqlite3.Error as exc:
        print(json.dumps({"error": str(exc)}), file=sys.stderr)
        sys.exit(1)

    if customer is None:
        print(
            json.dumps(
                {
                    "error": "Customer not found",
                    "customerusername": args.customerusername
                }
            ),
            file=sys.stderr
        )
        sys.exit(1)

    print(json.dumps(customer, indent=2))


if __name__ == "__main__":
    main()