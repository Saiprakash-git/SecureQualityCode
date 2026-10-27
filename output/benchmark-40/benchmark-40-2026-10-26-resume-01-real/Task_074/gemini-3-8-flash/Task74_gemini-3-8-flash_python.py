import os
from typing import Any, Optional
import psycopg2
from psycopg2.extensions import connection


def get_postgres_connection(
    dbname: str,
    user: str,
    password: str,
    host: str = "localhost",
    port: int = 5432,
    connect_timeout: int = 10,
    **kwargs: Any,
) -> connection:
    """
    Establishes and returns a connection to a PostgreSQL database.

    :param dbname: Name of the database to connect to.
    :param user: Username for authentication.
    :param password: Password for authentication.
    :param host: Host address of the PostgreSQL server.
    :param port: Port number of the PostgreSQL server.
    :param connect_timeout: Maximum wait for connection in seconds.
    :param kwargs: Additional connection parameters for psycopg2.connect.
    :return: An active psycopg2 connection object.
    :raises ConnectionError: If the connection attempt fails.
    """
    try:
        conn = psycopg2.connect(
            dbname=dbname,
            user=user,
            password=password,
            host=host,
            port=port,
            connect_timeout=connect_timeout,
            **kwargs,
        )
        return conn
    except psycopg2.Error as err:
        raise ConnectionError(f"Database connection failed: {err}") from err


if __name__ == "__main__":
    db = os.getenv("PGDATABASE", "postgres")
    usr = os.getenv("PGUSER", "postgres")
    pwd = os.getenv("PGPASSWORD", "postgres")
    h = os.getenv("PGHOST", "localhost")
    p = int(os.getenv("PGPORT", "5432"))

    try:
        conn_instance = get_postgres_connection(
            dbname=db,
            user=usr,
            password=pwd,
            host=h,
            port=p,
        )
        print("Successfully connected to the database.")
        conn_instance.close()
    except ConnectionError as e:
        print(e)