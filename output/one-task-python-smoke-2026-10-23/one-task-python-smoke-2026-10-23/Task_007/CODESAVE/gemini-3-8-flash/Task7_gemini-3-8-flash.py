import os
import sys
import json
import socket
import hashlib
import getpass
import threading


def hash_password(password: str) -> str:
    return hashlib.sha256(password.encode("utf-8")).hexdigest()


def initialize_user_store(filepath: str, initial_users: dict = None) -> None:
    if initial_users is None:
        initial_users = {
            "alice": hash_password("alice123"),
            "bob": hash_password("bob_secure"),
            "charlie": hash_password("password")
        }
    if not os.path.exists(filepath):
        with open(filepath, "w", encoding="utf-8") as f:
            json.dump(initial_users, f, indent=4)


def verify_credentials(filepath: str, username: str, password: str) -> bool:
    if not os.path.exists(filepath):
        return False
    try:
        with open(filepath, "r", encoding="utf-8") as f:
            users = json.load(f)
        stored_hash = users.get(username)
        if not stored_hash:
            return False
        return stored_hash == hash_password(password)
    except (json.JSONDecodeError, IOError):
        return False


class ChatServer:
    def __init__(self, host: str = "127.0.0.1", port: int = 8888, db_path: str = "users.json"):
        self.host = host
        self.port = port
        self.db_path = db_path
        self.server_socket = None
        self.is_running = False
        self.active_clients = {}
        self.lock = threading.Lock()
        initialize_user_store(self.db_path)

    def start(self) -> None:
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.bind((self.host, self.port))
        self.server_socket.listen(5)
        self.is_running = True
        print(f"[*] Server listening on {self.host}:{self.port}")

        try:
            while self.is_running:
                client_sock, client_addr = self.server_socket.accept()
                handler = threading.Thread(target=self.handle_client, args=(client_sock, client_addr), daemon=True)
                handler.start()
        except KeyboardInterrupt:
            self.stop()

    def stop(self) -> None:
        self.is_running = False
        if self.server_socket:
            self.server_socket.close()

    def handle_client(self, client_socket: socket.socket, client_address: tuple) -> None:
        authenticated_user = None
        buffer = ""
        try:
            while self.is_running:
                data = client_socket.recv(4096).decode("utf-8")
                if not data:
                    break
                buffer += data
                while "\n" in buffer:
                    line, buffer = buffer.split("\n", 1)
                    if not line.strip():
                        continue
                    response, authenticated_user = self.process_request(line, client_socket, authenticated_user)
                    client_socket.sendall((json.dumps(response) + "\n").encode("utf-8"))
        except (ConnectionResetError, BrokenPipeError):
            pass
        finally:
            with self.lock:
                if authenticated_user and authenticated_user in self.active_clients:
                    del self.active_clients[authenticated_user]
            client_socket.close()

    def process_request(self, raw_message: str, client_socket: socket.socket, current_user: str) -> tuple:
        try:
            payload = json.loads(raw_message)
        except json.JSONDecodeError:
            return {"status": "error", "message": "Invalid JSON format."}, current_user

        action = payload.get("action")

        if action == "login":
            username = payload.get("username", "").strip()
            password = payload.get("password", "")

            if not username or not password:
                return {"status": "error", "message": "Username and password required."}, current_user

            if verify_credentials(self.db_path, username, password):
                with self.lock:
                    self.active_clients[username] = client_socket
                return {"status": "success", "message": f"Welcome, {username}!"}, username
            else:
                return {"status": "error", "message": "Invalid credentials."}, current_user

        elif action == "message":
            if not current_user:
                return {"status": "error", "message": "Unauthorized. Please log in first."}, current_user
            msg_content = payload.get("content", "")
            self.broadcast(current_user, msg_content)
            return {"status": "success", "message": "Message sent."}, current_user

        elif action == "logout":
            with self.lock:
                if current_user in self.active_clients:
                    del self.active_clients[current_user]
            return {"status": "success", "message": "Logged out successfully."}, None

        return {"status": "error", "message": "Unknown action."}, current_user

    def broadcast(self, sender: str, message: str) -> None:
        broadcast_payload = json.dumps({
            "type": "broadcast",
            "sender": sender,
            "content": message
        }) + "\n"

        with self.lock:
            for user, sock in list(self.active_clients.items()):
                if user != sender:
                    try:
                        sock.sendall(broadcast_payload.encode("utf-8"))
                    except (BrokenPipeError, ConnectionResetError):
                        pass


class ChatClient:
    def __init__(self, host: str = "127.0.0.1", port: int = 8888):
        self.host = host
        self.port = port
        self.client_socket = None
        self.logged_in = False
        self.username = None

    def connect(self) -> bool:
        try:
            self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.client_socket.connect((self.host, self.port))
            return True
        except ConnectionRefusedError:
            print("[-] Unable to connect to server.")
            return False

    def build_login_request(self, username: str, password: str) -> str:
        request_data = {
            "action": "login",
            "username": username,
            "password": password
        }
        return json.dumps(request_data) + "\n"

    def prompt_and_login(self) -> bool:
        username = input("Username: ").strip()
        password = getpass.getpass("Password: ")

        request_packet = self.build_login_request(username, password)
        self.client_socket.sendall(request_packet.encode("utf-8"))

        response_line = self.read_response()
        if not response_line:
            print("[-] No response from server.")
            return False

        try:
            response = json.loads(response_line)
            if response.get("status") == "success":
                print(f"[+] {response.get('message')}")
                self.logged_in = True
                self.username = username
                return True
            else:
                print(f"[-] Login failed: {response.get('message')}")
                return False
        except json.JSONDecodeError:
            print("[-] Malformed server response.")
            return False

    def read_response(self) -> str:
        buffer = ""
        while "\n" not in buffer:
            chunk = self.client_socket.recv(1024).decode("utf-8")
            if not chunk:
                break
            buffer += chunk
        return buffer.strip()

    def receive_loop(self) -> None:
        buffer = ""
        while self.logged_in:
            try:
                data = self.client_socket.recv(1024).decode("utf-8")
                if not data:
                    break
                buffer += data
                while "\n" in buffer:
                    line, buffer = buffer.split("\n", 1)
                    if not line.strip():
                        continue
                    parsed = json.loads(line)
                    if parsed.get("type") == "broadcast":
                        print(f"\n[{parsed.get('sender')}]: {parsed.get('content')}\n> ", end="")
            except (ConnectionResetError, json.JSONDecodeError):
                break

    def run_chat(self) -> None:
        recv_thread = threading.Thread(target=self.receive_loop, daemon=True)
        recv_thread.start()

        print("[*] Type messages to chat, or '/logout' to exit.")
        while self.logged_in:
            try:
                msg = input("> ").strip()
                if not msg:
                    continue
                if msg.lower() == "/logout":
                    logout_payload = json.dumps({"action": "logout"}) + "\n"
                    self.client_socket.sendall(logout_payload.encode("utf-8"))
                    self.logged_in = False
                    break
                
                send_payload = json.dumps({"action": "message", "content": msg}) + "\n"
                self.client_socket.sendall(send_payload.encode("utf-8"))
            except (KeyboardInterrupt, EOFError):
                break
        self.client_socket.close()


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "server":
        server = ChatServer()
        server.start()
    else:
        client = ChatClient()
        if client.connect():
            if client.prompt_and_login():
                client.run_chat()