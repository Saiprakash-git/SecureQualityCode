import getpass
import hashlib
import json
import os
import socket
import sys
import threading


DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 9999
USER_DB_FILE = "users_db.json"


def hash_password(password: str) -> str:
    return hashlib.sha256(password.encode("utf-8")).hexdigest()


def init_storage(filepath: str) -> None:
    if not os.path.exists(filepath):
        default_users = {
            "alice": hash_password("alice123"),
            "bob": hash_password("bob123"),
            "charlie": hash_password("charlie123"),
        }
        with open(filepath, "w", encoding="utf-8") as f:
            json.dump(default_users, f, indent=4)


def load_credentials(filepath: str) -> dict:
    if not os.path.exists(filepath):
        init_storage(filepath)
    with open(filepath, "r", encoding="utf-8") as f:
        return json.load(f)


def authenticate_user(filepath: str, username: str, password: str) -> bool:
    users = load_credentials(filepath)
    if username not in users:
        return False
    return users[username] == hash_password(password)


def send_packet(sock_file, payload: dict) -> None:
    data = json.dumps(payload) + "\n"
    sock_file.write(data)
    sock_file.flush()


def receive_packet(sock_file) -> dict:
    line = sock_file.readline()
    if not line:
        return {}
    return json.loads(line.strip())


def broadcast_message(active_clients: dict, lock: threading.Lock, sender: str, message: str) -> None:
    payload = {
        "status": "broadcast",
        "sender": sender,
        "message": message,
    }
    with lock:
        for user, writer in list(active_clients.items()):
            try:
                send_packet(writer, payload)
            except Exception:
                pass


def process_request(request: dict, db_path: str, current_user: str, active_clients: dict, lock: threading.Lock, writer) -> tuple:
    action = request.get("action")
    
    if action == "login":
        username = request.get("username", "").strip()
        password = request.get("password", "")
        
        if not username or not password:
            return {"status": "error", "message": "Missing credentials"}, current_user

        if authenticate_user(db_path, username, password):
            with lock:
                if username in active_clients:
                    return {"status": "error", "message": "User already logged in"}, current_user
                active_clients[username] = writer
            return {"status": "ok", "message": "Login successful", "username": username}, username
        else:
            return {"status": "error", "message": "Invalid username or password"}, current_user

    elif action == "msg":
        if not current_user:
            return {"status": "error", "message": "Unauthorized"}, current_user
        
        content = request.get("content", "").strip()
        if content:
            broadcast_message(active_clients, lock, current_user, content)
            return {"status": "ok", "message": "Message delivered"}, current_user
        return {"status": "error", "message": "Empty message"}, current_user

    elif action == "logout":
        return {"status": "ok", "message": "Logged out"}, None

    else:
        return {"status": "error", "message": "Unknown action"}, current_user


def handle_client_connection(client_sock: socket.socket, addr: tuple, db_path: str, active_clients: dict, lock: threading.Lock) -> None:
    reader = client_sock.makefile("r", encoding="utf-8")
    writer = client_sock.makefile("w", encoding="utf-8")
    current_user = None

    try:
        while True:
            request = receive_packet(reader)
            if not request:
                break
            
            response, updated_user = process_request(
                request, db_path, current_user, active_clients, lock, writer
            )
            current_user = updated_user
            send_packet(writer, response)
            
            if request.get("action") == "logout":
                break
    except (ConnectionResetError, BrokenPipeError):
        pass
    finally:
        if current_user:
            with lock:
                if current_user in active_clients:
                    del active_clients[current_user]
            broadcast_message(active_clients, lock, "SYSTEM", f"{current_user} has left the chat.")
        try:
            reader.close()
            writer.close()
            client_sock.close()
        except Exception:
            pass


def run_server(host: str, port: int, db_path: str) -> None:
    init_storage(db_path)
    active_clients = {}
    lock = threading.Lock()

    server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_sock.bind((host, port))
    server_sock.listen(128)
    print(f"[*] Server listening on {host}:{port}")

    try:
        while True:
            client_sock, addr = server_sock.accept()
            t = threading.Thread(
                target=handle_client_connection,
                args=(client_sock, addr, db_path, active_clients, lock),
                daemon=True,
            )
            t.start()
    except KeyboardInterrupt:
        print("\n[*] Server shutting down.")
    finally:
        server_sock.close()


def prompt_user_credentials() -> tuple:
    print("--- User Authentication ---")
    username = input("Enter username: ").strip()
    try:
        password = getpass.getpass("Enter password: ")
    except Exception:
        password = input("Enter password: ")
    return username, password


def build_login_payload(username: str, password: str) -> dict:
    return {
        "action": "login",
        "username": username,
        "password": password,
    }


def build_chat_payload(content: str) -> dict:
    return {
        "action": "msg",
        "content": content,
    }


def listen_server_broadcasts(reader, stop_event: threading.Event) -> None:
    while not stop_event.is_set():
        try:
            packet = receive_packet(reader)
            if not packet:
                print("\n[!] Connection closed by server.")
                stop_event.set()
                break
            
            if packet.get("status") == "broadcast":
                sender = packet.get("sender")
                message = packet.get("message")
                print(f"\n[{sender}]: {message}\n> ", end="", flush=True)
        except Exception:
            break


def run_client(host: str, port: int) -> None:
    client_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        client_sock.connect((host, port))
    except ConnectionRefusedError:
        print(f"[!] Unable to connect to server at {host}:{port}")
        return

    reader = client_sock.makefile("r", encoding="utf-8")
    writer = client_sock.makefile("w", encoding="utf-8")

    authenticated = False
    username = ""

    while not authenticated:
        username, password = prompt_user_credentials()
        login_req = build_login_payload(username, password)
        send_packet(writer, login_req)
        
        response = receive_packet(reader)
        if response.get("status") == "ok":
            print(f"[+] {response.get('message')}")
            authenticated = True
        else:
            print(f"[-] Authentication failed: {response.get('message')}")
            retry = input("Try again? (y/n): ").strip().lower()
            if retry != 'y':
                reader.close()
                writer.close()
                client_sock.close()
                return

    stop_event = threading.Event()
    listener_thread = threading.Thread(
        target=listen_server_broadcasts,
        args=(reader, stop_event),
        daemon=True,
    )
    listener_thread.start()

    print(f"[*] Logged in as '{username}'. Type messages below. Type '/quit' to exit.")
    try:
        while not stop_event.is_set():
            msg = input("> ")
            if not msg:
                continue
            if msg.strip() == "/quit":
                send_packet(writer, {"action": "logout"})
                break
            chat_req = build_chat_payload(msg)
            send_packet(writer, chat_req)
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        stop_event.set()
        try:
            reader.close()
            writer.close()
            client_sock.close()
        except Exception:
            pass
        print("\n[*] Disconnected from chat server.")


def main() -> None:
    if len(sys.argv) < 2:
        print("Usage:")
        print("  python script.py server [host] [port]")
        print("  python script.py client [host] [port]")
        sys.exit(1)

    mode = sys.argv[1].lower()
    host = sys.argv[2] if len(sys.argv) > 2 else DEFAULT_HOST
    port = int(sys.argv[3]) if len(sys.argv) > 3 else DEFAULT_PORT

    if mode == "server":
        run_server(host, port, USER_DB_FILE)
    elif mode == "client":
        run_client(host, port)
    else:
        print(f"Unknown mode: {mode}. Choose 'server' or 'client'.")
        sys.exit(1)


if __name__ == "__main__":
    main()