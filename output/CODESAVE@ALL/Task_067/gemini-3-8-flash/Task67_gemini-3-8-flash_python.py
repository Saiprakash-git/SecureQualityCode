import argparse
from dataclasses import asdict, dataclass
import json
import sqlite3
import sys
from typing import Any, Dict, Optional


@dataclass
class Customer:
    id: int
    customerusername: str
    first_name: str
    last_name: str
    email: str
    phone: Optional[str]
    created_at: str

    def to_dict(self) -> Dict[str, Any]:
        return asdict(self)


def init_db(db_path: str = "customers.db") -> None:
    with sqlite3.connect(db_path) as conn:
        cursor = conn.cursor()
        cursor.execute(
            """
            CREATE TABLE IF NOT EXISTS customer (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                customerusername TEXT UNIQUE NOT NULL,
                first_name TEXT NOT NULL,
                last_name TEXT NOT NULL,
                email TEXT NOT NULL,
                phone TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
            """
        )
        cursor.execute(
            """
            INSERT OR IGNORE INTO customer (customerusername, first_name, last_name, email, phone)
            VALUES 
                ('jdoe', 'John', 'Doe', 'jdoe@example.com', '555-0101'),
                ('asmith', 'Alice', 'Smith', 'asmith@example.com', '555-0102')
            """
        )
        conn.commit()


def get_customer_by_username(
    customerusername: str, db_path: str = "customers.db"
) -> Optional[Customer]:
    query = """
        SELECT id, customerusername, first_name, last_name, email, phone, created_at
        FROM customer
        WHERE customerusername = ?
    """
    with sqlite3.connect(db_path) as conn:
        conn.row_factory = sqlite3.Row
        cursor = conn.cursor()
        cursor.execute(query, (customerusername,))
        row = cursor.fetchone()

        if row is None:
            return None

        return Customer(
            id=row["id"],
            customerusername=row["customerusername"],
            first_name=row["first_name"],
            last_name=row["last_name"],
            email=row["email"],
            phone=row["phone"],
            created_at=str(row["created_at"]),
        )


def parse_arguments(args: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Retrieve customer information by customer username."
    )
    parser.add_argument(
        "customerusername",
        type=str,
        help="The username of the customer to query.",
    )
    parser.add_argument(
        "--db",
        dest="db_path",
        type=str,
        default="customers.db",
        help="Path to the SQLite database file (default: customers.db).",
    )
    return parser.parse_args(args)


def main() -> None:
    parsed_args = parse_arguments(sys.argv[1:])
    init_db(parsed_args.db_path)
    
    customer = get_customer_by_username(
        customerusername=parsed_args.customerusername,
        db_path=parsed_args.db_path,
    )

    if customer:
        print(json.dumps(customer.to_dict(), indent=2))
    else:
        print(
            json.dumps(
                {"error": f"Customer '{parsed_args.customerusername}' not found."},
                indent=2,
            )
        )
        sys.exit(1)


if __name__ == "__main__":
    main()