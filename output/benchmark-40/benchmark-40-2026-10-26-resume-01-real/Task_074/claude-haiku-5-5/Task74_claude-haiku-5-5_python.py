import os
import psycopg2


def create_connection(dbname=None, user=None, password=None, host=None, port=None):
    dbname = dbname or os.getenv("PGDATABASE", "postgres")
    user = user or os.getenv("PGUSER", "postgres")
    password = password or os.getenv("PGPASSWORD", "")
    host = host or os.getenv("PGHOST", "localhost")
    port = port or os.getenv("PGPORT", "5432")

    try:
        connection = psycopg2.connect(
            dbname=dbname,
            user=user,
            password=password,
            host=host,
            port=port,
        )
        return connection
    except psycopg2.Error as e:
        raise ConnectionError(f"Unable to connect to PostgreSQL database: {e}") from e


if __name__ == "__main__":
    conn = create_connection()
    print("Connection established:", not conn.closed)
    conn.close()