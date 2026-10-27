import os
import psycopg2

def create_connection(
    host=None,
    database=None,
    user=None,
    password=None,
    port=None,
):
    host = host or os.getenv("PGHOST", "localhost")
    database = database or os.getenv("PGDATABASE", "postgres")
    user = user or os.getenv("PGUSER", "postgres")
    password = password or os.getenv("PGPASSWORD", "")
    port = port or os.getenv("PGPORT", "5432")

    return psycopg2.connect(
        host=host,
        database=database,
        user=user,
        password=password,
        port=port,
    )