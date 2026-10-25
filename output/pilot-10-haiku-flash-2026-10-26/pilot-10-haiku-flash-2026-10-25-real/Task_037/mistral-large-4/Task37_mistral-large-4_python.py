import sqlite3

def connect_database(db_name):
    return sqlite3.connect(db_name)

def initialize_database(conn):
    cursor = conn.cursor()
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            email TEXT,
            full_name TEXT,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    ''')
    sample_data = [
        ('john_doe', 'john@example.com', 'John Doe'),
        ('jane_smith', 'jane@example.com', 'Jane Smith'),
        ('admin_user', 'admin@example.com', 'Administrator')
    ]
    for username, email, full_name in sample_data:
        try:
            cursor.execute('INSERT INTO users (username, email, full_name) VALUES (?, ?, ?)',
                          (username, email, full_name))
        except sqlite3.IntegrityError:
            pass
    conn.commit()
    cursor.close()

def query_user_by_username(conn, username):
    cursor = conn.cursor()
    cursor.execute('SELECT id, username, email, full_name, created_at FROM users WHERE username = ?',
                   (username,))
    result = cursor.fetchone()
    cursor.close()
    return result

def main():
    db_name = 'users.db'
    conn = connect_database(db_name)
    initialize_database(conn)

    username_input = input('Enter username: ')
    user_record = query_user_by_username(conn, username_input)

    if user_record:
        print(f'ID: {user_record[0]}')
        print(f'Username: {user_record[1]}')
        print(f'Email: {user_record[2]}')
        print(f'Full Name: {user_record[3]}')
        print(f'Created At: {user_record[4]}')
    else:
        print('No user found with that username.')

    conn.close()

if __name__ == '__main__':
    main()