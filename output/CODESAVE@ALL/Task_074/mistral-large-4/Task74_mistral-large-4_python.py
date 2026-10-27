import psycopg2

def create_postgres_connection(host, database, user, password, port=5432):
    connection = psycopg2.connect(
        host=host,
        database=database,
        user=user,
        password=password,
        port=port
    )
    return connection