import argparse
import json
import os
import sqlite3
from typing import Any, Optional


def get_customer(customerusername: str, database_path: str = "customers.db") -> Optional[dict[str, Any]]:
    """Return the matching customer row as a dictionary, or None if not found."""
    with sqlite3.connect(database_path) as connection:
        connection.row_factory = sqlite3.Row
        row = connection.execute(
            "SELECT * FROM customer WHERE customerusername = ? LIMIT 1",
            (customerusername,),
        ).fetchone()
        return dict(row) if row is not None else None


def main(argv: Optional[list[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="Retrieve a customer by username.")
    parser.add_argument("customerusername", help="Username of the customer to retrieve")
    parser.add_argument(
        "--database",
        default=os.environ.get("CUSTOMER_DATABASE", "customers.db"),
        help="Path to the SQLite database (default: customers.db or CUSTOMER_DATABASE)",
    )
    args = parser.parse_args(argv)

    customer = get_customer(args.customerusername, args.database)
    print(json.dumps(customer, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())