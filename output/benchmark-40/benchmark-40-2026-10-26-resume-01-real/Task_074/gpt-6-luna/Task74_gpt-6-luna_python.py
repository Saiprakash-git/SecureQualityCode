import psycopg2


def create_postgres_connection(
    host,
    database,
    user,
    password,
    port=5432,
    **options,
):
    """Create and return a connection to a PostgreSQL database."""
    return psycopg2.connect(
        host=host,
        database=database,
        user=user,
        password=password,
        port=port,
        **options,
    )