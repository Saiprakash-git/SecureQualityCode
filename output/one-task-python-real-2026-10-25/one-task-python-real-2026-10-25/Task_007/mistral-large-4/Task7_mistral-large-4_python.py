import socket
import json
import sys
import os
import threading

HOST = '127.0.0.1'
PORT = 65432
USERS_FILE = 'users.json'

def load_users(file_path):
    if not os.path.exists(file_path):
        default_users = {"admin": "password123", "user1": "pass1"}
        with open(file_path, 'w') as f:
            json.dump(default_users, f)
        return default_users
    with open(file_path, 'r') as f:
        return json.load(f)

def authenticate(username, password, users):
    return users.get(username) == password

def parse_message(data):
    try:
        return json.loads(data.decode('utf-8'))
    except (json.JSONDecodeError, UnicodeDecodeError):
        return None

def build_response(success, message, data=None):
    response = {"success": success, "message": message}
    if data is not None:
        response["data"] = data
    return response

def execute_action(message, users, authenticated, username):
    msg_type = message.get("type")

    if not authenticated:
        if msg_type == "login":
            user = message.get("username")
            pwd = message.get("password")
            if authenticate(user, pwd, users):
                return build_response(True, "Login successful", {"username": user}), True, user
            else:
                return build_response(False, "Invalid credentials"), False, None
        else:
            return build_response(False, "Authentication required"), False, None
    else:
        if msg_type == "message":
            content = message.get("content")
            return build_response(True, f"Message received: {content}"), True, username
        elif msg_type == "logout":
            return build_response(True, "Logged out"), False, None
        else:
            return build_response(False, "Unknown command"), True, username

def handle_client(conn, addr, users):
    print(f"New connection from {addr}")
    authenticated = False
    username = None

    try:
        while True:
            data = conn.recv(1024)
            if not data:
                break

            message = parse_message(data)
            if message is None:
                response = build_response(False, "Invalid message format")
                conn.send(json.dumps(response).encode('utf-8'))
                continue

            response, authenticated, username = execute_action(message, users, authenticated, username)
            conn.send(json.dumps(response).encode('utf-8'))

            if message.get("type") == "logout":
                break

    except ConnectionResetError:
        print(f"Connection reset by {addr}")
    except Exception as e:
        print(f"Error handling {addr}: {e}")
    finally:
        conn.close()
        print(f"Connection closed: {addr}")

def start_server(host, port, users_file):
    users = load_users(users_file)
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((host, port))
    server_socket.listen()
    print(f"Server listening on {host}:{port}")

    try:
        while True:
            conn, addr = server_socket.accept()
            thread = threading.Thread(target=handle_client, args=(conn, addr, users))
            thread.daemon = True
            thread.start()
    except KeyboardInterrupt:
        print("\nServer shutting down")
    finally:
        server_socket.close()

def capture_credentials():
    username = input("Enter username: ")
    password = input("Enter password: ")
    return username, password

def construct_login_request(username, password):
    return {
        "type": "login",
        "username": username,
        "password": password
    }

def construct_chat_message(content):
    return {
        "type": "message",
        "content": content
    }

def transmit_request(sock, request):
    sock.send(json.dumps(request).encode('utf-8'))

def receive_response(sock):
    data = sock.recv(1024)
    if not data:
        return None
    return json.loads(data.decode('utf-8'))

def start_client(host, port):
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        client_socket.connect((host, port))
        print(f"Connected to server at {host}:{port}")

        username, password = capture_credentials()
        login_request = construct_login_request(username, password)
        transmit_request(client_socket, login_request)

        response = receive_response(client_socket)
        if response is None:
            print("No response from server")
            return

        print(f"Server: {response.get('message')}")

        if not response.get('success'):
            return

        print("Login successful. Enter messages (type 'logout' to exit):")

        while True:
            content = input()
            if content.lower() == 'logout':
                logout_msg = {"type": "logout"}
                transmit_request(client_socket, logout_msg)
                response = receive_response(client_socket)
                if response:
                    print(f"Server: {response.get('message')}")
                break

            chat_message = construct_chat_message(content)
            transmit_request(client_socket, chat_message)
            response = receive_response(client_socket)
            if response:
                print(f"Server: {response.get('message')}")

    except ConnectionRefusedError:
        print("Could not connect to server")
    except Exception as e:
        print(f"Client error: {e}")
    finally:
        client_socket.close()

if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "server":
        start_server(HOST, PORT, USERS_FILE)
    else:
        start_client(HOST, PORT)