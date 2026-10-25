import argparse
import hashlib
import hmac
import json
import os
import secrets
import socket
import threading
from typing import Callable, Dict, Optional


PBKDF2_ITERATIONS = 200_000
MAX_MESSAGE_LENGTH = 4096


def _derive_password(password: str, salt: bytes, iterations: int) -> bytes:
    return hashlib.pbkdf2_hmac(
        "sha256", password.encode("utf-8"), salt, iterations
    )


def create_user_store(path: str, users: Dict[str, str]) -> None:
    """Create or replace a JSON user store from {username: password}."""
    records = {}
    for username, password in users.items():
        salt = secrets.token_bytes(16)
        records[username] = {
            "salt": salt.hex(),
            "password_hash": _derive_password(
                password, salt, PBKDF2_ITERATIONS
            ).hex(),
            "iterations": PBKDF2_ITERATIONS,
        }

    directory = os.path.dirname(os.path.abspath(path))
    os.makedirs(directory, exist_ok=True)
    temporary_path = path + ".tmp"
    with open(temporary_path, "w", encoding="utf-8") as file:
        json.dump(records, file)
    os.replace(temporary_path, path)


def authenticate_user(path: str, username: str, password: str) -> bool:
    """Authenticate credentials against the JSON user store."""
    try:
        with open(path, "r", encoding="utf-8") as file:
            records = json.load(file)
        record = records.get(username)
        if not isinstance(record, dict):
            return False

        salt = bytes.fromhex(record["salt"])
        expected = bytes.fromhex(record["password_hash"])
        iterations = int(record.get("iterations", PBKDF2_ITERATIONS))
        if iterations <= 0:
            return False

        actual = _derive_password(password, salt, iterations)
        return hmac.compare_digest(actual, expected)
    except (OSError, ValueError, TypeError, KeyError, json.JSONDecodeError):
        return False


def build_request(action: str, **fields) -> dict:
    """Build a protocol request."""
    return {"action": action, **fields}


def _send_message(sock: socket.socket, message: dict, lock=None) -> None:
    data = (json.dumps(message, ensure_ascii=False) + "\n").encode("utf-8")
    if lock is None:
        sock.sendall(data)
    else:
        with lock:
            sock.sendall(data)


def _read_message(reader) -> Optional[dict]:
    line = reader.readline(MAX_MESSAGE_LENGTH + 1)
    if not line:
        return None
    if len(line) > MAX_MESSAGE_LENGTH or not line.endswith(b"\n"):
        raise ValueError("Message is too large or not newline-terminated")
    message = json.loads(line.decode("utf-8"))
    if not isinstance(message, dict):
        raise ValueError("Request must be a JSON object")
    return message


class ChatServer:
    def __init__(self, host: str, port: int, user_store_path: str):
        self.host = host
        self.port = port
        self.user_store_path = user_store_path
        self._clients = {}
        self._clients_lock = threading.Lock()
        self._stop_event = threading.Event()
        self._server_socket = None

    def serve_forever(self) -> None:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server:
            self._server_socket = server
            server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            server.bind((self.host, self.port))
            server.listen()
            server.settimeout(1.0)

            while not self._stop_event.is_set():
                try:
                    client_socket, _ = server.accept()
                except socket.timeout:
                    continue
                threading.Thread(
                    target=self._handle_client,
                    args=(client_socket,),
                    daemon=True,
                ).start()

    def stop(self) -> None:
        self._stop_event.set()
        if self._server_socket is not None:
            try:
                self._server_socket.close()
            except OSError:
                pass

    def _send(self, client_socket: socket.socket, message: dict, lock) -> None:
        try:
            _send_message(client_socket, message, lock)
        except OSError:
            pass

    def _broadcast(self, message: dict) -> None:
        with self._clients_lock:
            recipients = list(self._clients.values())
        for client_socket, send_lock in recipients:
            self._send(client_socket, message, send_lock)

    def _handle_client(self, client_socket: socket.socket) -> None:
        send_lock = threading.Lock()
        username = None

        try:
            client_socket.settimeout(None)
            with client_socket, client_socket.makefile("rb") as reader:
                while True:
                    try:
                        request = _read_message(reader)
                    except (ValueError, UnicodeDecodeError, json.JSONDecodeError):
                        self._send(
                            client_socket,
                            {"type": "error", "message": "Invalid request"},
                            send_lock,
                        )
                        break

                    if request is None:
                        break

                    action = request.get("action")

                    if action == "login":
                        if username is not None:
                            response = {
                                "type": "error",
                                "message": "Already logged in",
                            }
                        else:
                            requested_username = request.get("username")
                            password = request.get("password")
                            if (
                                not isinstance(requested_username, str)
                                or not isinstance(password, str)
                                or not requested_username
                                or len(requested_username) > 64
                            ):
                                response = {
                                    "type": "login",
                                    "success": False,
                                    "message": "Invalid credentials",
                                }
                            elif not authenticate_user(
                                self.user_store_path,
                                requested_username,
                                password,
                            ):
                                response = {
                                    "type": "login",
                                    "success": False,
                                    "message": "Invalid username or password",
                                }
                            else:
                                username = requested_username
                                with self._clients_lock:
                                    self._clients[username] = (
                                        client_socket,
                                        send_lock,
                                    )
                                response = {
                                    "type": "login",
                                    "success": True,
                                    "username": username,
                                    "message": "Login successful",
                                }
                        self._send(client_socket, response, send_lock)

                    elif action == "chat":
                        text = request.get("message")
                        if username is None:
                            response = {
                                "type": "error",
                                "message": "Please log in first",
                            }
                        elif (
                            not isinstance(text, str)
                            or not text.strip()
                            or len(text) > MAX_MESSAGE_LENGTH
                        ):
                            response = {
                                "type": "error",
                                "message": "Invalid chat message",
                            }
                        else:
                            self._broadcast(
                                {
                                    "type": "chat",
                                    "username": username,
                                    "message": text,
                                }
                            )
                            continue
                        self._send(client_socket, response, send_lock)

                    elif action == "logout":
                        self._send(
                            client_socket,
                            {"type": "logout", "success": True},
                            send_lock,
                        )
                        break

                    else:
                        self._send(
                            client_socket,
                            {"type": "error", "message": "Unknown action"},
                            send_lock,
                        )
        except (OSError, ValueError, UnicodeDecodeError):
            pass
        finally:
            if username is not None:
                with self._clients_lock:
                    current = self._clients.get(username)
                    if current and current[0] is client_socket:
                        del self._clients[username]
            try:
                client_socket.close()
            except OSError:
                pass


def client_component(
    host: str,
    port: int,
    username: Optional[str] = None,
    password: Optional[str] = None,
    input_fn: Callable[[str], str] = input,
    output_fn: Callable[[str], None] = print,
) -> bool:
    """Prompt for credentials if needed, log in, and run the chat client."""
    if username is None:
        username = input_fn("Username: ")
    if password is None:
        password = input_fn("Password: ")

    try:
        sock = socket.create_connection((host, port))
        reader = sock.makefile("rb")
    except OSError as error:
        output_fn(f"Connection failed: {error}")
        return False

    try:
        login_request = build_request(
            "login", username=username, password=password
        )
        _send_message(sock, login_request)
        response = _read_message(reader)

        if not response or not response.get("success"):
            output_fn(
                response.get("message", "Login failed")
                if response
                else "Server disconnected"
            )
            return False

        output_fn(response.get("message", "Login successful"))

        def receive_messages() -> None:
            try:
                while True:
                    message = _read_message(reader)
                    if message is None:
                        break
                    message_type = message.get("type")
                    if message_type == "chat":
                        output_fn(
                            f'{message.get("username", "?")}: '
                            f'{message.get("message", "")}'
                        )
                    elif message_type == "error":
                        output_fn(f'Error: {message.get("message", "")}')
            except (OSError, ValueError, UnicodeDecodeError, json.JSONDecodeError):
                pass

        receiver = threading.Thread(target=receive_messages, daemon=True)
        receiver.start()

        while True:
            try:
                text = input_fn("")
            except (EOFError, KeyboardInterrupt):
                break
            if text.strip().lower() in {"/quit", "/exit"}:
                break
            if text.strip():
                _send_message(sock, build_request("chat", message=text))

        try:
            _send_message(sock, build_request("logout"))
        except OSError:
            pass
        return True
    except (OSError, ValueError, UnicodeDecodeError, json.JSONDecodeError) as error:
        output_fn(f"Chat connection ended: {error}")
        return False
    finally:
        try:
            reader.close()
        except OSError:
            pass
        try:
            sock.close()
        except OSError:
            pass


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description="Simple authenticated chat")
    subparsers = parser.add_subparsers(dest="command", required=True)

    server_parser = subparsers.add_parser("server")
    server_parser.add_argument("--host", default="127.0.0.1")
    server_parser.add_argument("--port", type=int, default=5000)
    server_parser.add_argument("--users", required=True)

    client_parser = subparsers.add_parser("client")
    client_parser.add_argument("--host", default="127.0.0.1")
    client_parser.add_argument("--port", type=int, default=5000)

    create_parser = subparsers.add_parser("create-users")
    create_parser.add_argument("--users", required=True)
    create_parser.add_argument(
        "account",
        nargs="+",
        help="Accounts in username=password form",
    )

    args = parser.parse_args(argv)

    if args.command == "server":
        try:
            ChatServer(args.host, args.port, args.users).serve_forever()
        except KeyboardInterrupt:
            return 0
    elif args.command == "client":
        return 0 if client_component(args.host, args.port) else 1
    elif args.command == "create-users":
        users = {}
        for account in args.account:
            if "=" not in account:
                parser.error("Accounts must use username=password form")
            username, password = account.split("=", 1)
            if not username or not password:
                parser.error("Username and password must not be empty")
            users[username] = password
        create_user_store(args.users, users)
        return 0

    return 0


if __name__ == "__main__":
    raise SystemExit(main())