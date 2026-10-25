import argparse
import getpass
import hashlib
import hmac
import json
import os
import queue
import re
import secrets
import shlex
import socket
import socketserver
import ssl
import sys
import tempfile
import threading
from datetime import datetime, timezone
from pathlib import Path
from typing import Any


MAX_LINE_BYTES = 65536
PASSWORD_ITERATIONS = 310000
USERNAME_PATTERN = re.compile(r"^[A-Za-z0-9_.-]{1,32}$")


def derive_password_hash(password: str, salt: bytes, iterations: int) -> bytes:
    return hashlib.pbkdf2_hmac("sha256", password.encode("utf-8"), salt, iterations)


class AccountStore:
    def __init__(self, path: str | Path):
        self.path = Path(path)
        self.lock = threading.RLock()
        self.dummy_salt = bytes.fromhex("00112233445566778899aabbccddeeff")
        self.dummy_hash = derive_password_hash("not-a-real-password", self.dummy_salt, PASSWORD_ITERATIONS)
        self.path.parent.mkdir(parents=True, exist_ok=True)
        if not self.path.exists():
            self._write_accounts({})

    def _read_accounts(self) -> dict[str, dict[str, Any]]:
        try:
            with self.path.open("r", encoding="utf-8") as file:
                data = json.load(file)
        except (OSError, json.JSONDecodeError) as exc:
            raise RuntimeError(f"Could not read account file {self.path}: {exc}") from exc
        if not isinstance(data, dict):
            raise RuntimeError("Account file must contain a JSON object")
        return data

    def _write_accounts(self, accounts: dict[str, dict[str, Any]]) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        fd, temporary_path = tempfile.mkstemp(prefix=f".{self.path.name}.", dir=str(self.path.parent))
        try:
            with os.fdopen(fd, "w", encoding="utf-8") as file:
                json.dump(accounts, file, indent=2, sort_keys=True)
                file.write("\n")
                file.flush()
                os.fsync(file.fileno())
            try:
                os.chmod(temporary_path, 0o600)
            except OSError:
                pass
            os.replace(temporary_path, self.path)
        finally:
            try:
                os.unlink(temporary_path)
            except FileNotFoundError:
                pass

    def add_user(self, username: str, password: str) -> None:
        if not USERNAME_PATTERN.fullmatch(username):
            raise ValueError("Username must be 1–32 characters: letters, digits, '.', '_' or '-'.")
        if not password:
            raise ValueError("Password must not be empty.")
        with self.lock:
            accounts = self._read_accounts()
            if username in accounts:
                raise ValueError(f"User {username!r} already exists.")
            salt = secrets.token_bytes(16)
            password_hash = derive_password_hash(password, salt, PASSWORD_ITERATIONS)
            accounts[username] = {
                "salt": salt.hex(),
                "hash": password_hash.hex(),
                "iterations": PASSWORD_ITERATIONS,
            }
            self._write_accounts(accounts)

    def verify(self, username: str, password: str) -> bool:
        with self.lock:
            accounts = self._read_accounts()
            record = accounts.get(username)
        if not isinstance(record, dict):
            candidate = derive_password_hash(password, self.dummy_salt, PASSWORD_ITERATIONS)
            hmac.compare_digest(candidate, self.dummy_hash)
            return False
        try:
            salt = bytes.fromhex(record["salt"])
            expected = bytes.fromhex(record["hash"])
            iterations = int(record["iterations"])
            if not salt or not expected or iterations < 10000:
                raise ValueError("Invalid password record")
            candidate = derive_password_hash(password, salt, iterations)
        except (KeyError, TypeError, ValueError):
            candidate = derive_password_hash(password, self.dummy_salt, PASSWORD_ITERATIONS)
            hmac.compare_digest(candidate, self.dummy_hash)
            return False
        return hmac.compare_digest(candidate, expected)


class ChatTCPServer(socketserver.ThreadingTCPServer):
    allow_reuse_address = True
    daemon_threads = True

    def __init__(
        self,
        address: tuple[str, int],
        account_store: AccountStore,
        tls_context: ssl.SSLContext | None = None,
    ):
        self.account_store = account_store
        self.tls_context = tls_context
        self.sessions_lock = threading.RLock()
        self.sessions: dict[str, set["ChatRequestHandler"]] = {}
        super().__init__(address, ChatRequestHandler)

    def get_request(self) -> tuple[socket.socket, Any]:
        connection, address = super().get_request()
        if self.tls_context is None:
            return connection, address
        try:
            connection = self.tls_context.wrap_socket(connection, server_side=True)
        except Exception:
            connection.close()
            raise
        return connection, address

    def add_session(self, username: str, handler: "ChatRequestHandler") -> None:
        with self.sessions_lock:
            self.sessions.setdefault(username, set()).add(handler)

    def remove_session(self, username: str, handler: "ChatRequestHandler") -> None:
        with self.sessions_lock:
            handlers = self.sessions.get(username)
            if handlers is None:
                return
            handlers.discard(handler)
            if not handlers:
                self.sessions.pop(username, None)

    def online_usernames(self) -> list[str]:
        with self.sessions_lock:
            return sorted(name for name, handlers in self.sessions.items() if handlers)

    def send_to_user(self, username: str, event: dict[str, Any]) -> bool:
        with self.sessions_lock:
            handlers = list(self.sessions.get(username, ()))
        sent = False
        for handler in handlers:
            sent = handler.send_message(event) or sent
        return sent

    def broadcast(self, event: dict[str, Any]) -> None:
        with self.sessions_lock:
            handlers = [handler for group in self.sessions.values() for handler in group]
        for handler in handlers:
            handler.send_message(event)


class ChatRequestHandler(socketserver.StreamRequestHandler):
    def setup(self) -> None:
        super().setup()
        self.username: str | None = None
        self.send_lock = threading.Lock()

    def send_message(self, message: dict[str, Any]) -> bool:
        try:
            encoded = json.dumps(message, separators=(",", ":"), ensure_ascii=False).encode("utf-8") + b"\n"
            if len(encoded) > MAX_LINE_BYTES:
                return False
            with self.send_lock:
                self.wfile.write(encoded)
                self.wfile.flush()
            return True
        except (OSError, ValueError):
            return False

    def handle(self) -> None:
        while True:
            try:
                raw_line = self.rfile.readline(MAX_LINE_BYTES + 1)
            except OSError:
                break
            if not raw_line:
                break
            if len(raw_line) > MAX_LINE_BYTES:
                self.send_message({"id": None, "ok": False, "error": "Request is too large."})
                break
            try:
                request = json.loads(raw_line.decode("utf-8"))
            except (UnicodeDecodeError, json.JSONDecodeError):
                self.send_message({"id": None, "ok": False, "error": "Request must be valid JSON."})
                continue
            if not isinstance(request, dict):
                self.send_message({"id": None, "ok": False, "error": "Request must be a JSON object."})
                continue

            request_id = request.get("id")
            response, should_close = self.process_request(request)
            response["id"] = request_id
            if not self.send_message(response) or should_close:
                break

    def process_request(self, request: dict[str, Any]) -> tuple[dict[str, Any], bool]:
        action = request.get("action")
        if not isinstance(action, str):
            return {"ok": False, "error": "Missing or invalid action."}, False

        if self.username is None:
            if action != "login":
                return {"ok": False, "error": "Please log in first."}, False
            username = request.get("username")
            password = request.get("password")
            if not isinstance(username, str) or not USERNAME_PATTERN.fullmatch(username):
                return {"ok": False, "error": "Invalid username or password."}, False
            if not isinstance(password, str) or not self.server.account_store.verify(username, password):
                return {"ok": False, "error": "Invalid username or password."}, False
            self.username = username
            self.server.add_session(username, self)
            return {"ok": True, "message": f"Logged in as {username}."}, False

        if action == "users":
            return {"ok": True, "users": self.server.online_usernames()}, False

        if action == "say":
            text = request.get("text")
            if not isinstance(text, str) or not text.strip():
                return {"ok": False, "error": "Message must not be empty."}, False
            if len(text) > 4000:
                return {"ok": False, "error": "Message is too long (maximum 4000 characters)."}, False
            event = {
                "event": "chat",
                "kind": "public",
                "from": self.username,
                "text": text,
                "time": datetime.now(timezone.utc).isoformat(),
            }
            self.server.broadcast(event)
            return {"ok": True, "message": "Message sent."}, False

        if action == "dm":
            recipient = request.get("to")
            text = request.get("text")
            if not isinstance(recipient, str) or not USERNAME_PATTERN.fullmatch(recipient):
                return {"ok": False, "error": "Invalid recipient username."}, False
            if not isinstance(text, str) or not text.strip():
                return {"ok": False, "error": "Message must not be empty."}, False
            if len(text) > 4000:
                return {"ok": False, "error": "Message is too long (maximum 4000 characters)."}, False
            event = {
                "event": "chat",
                "kind": "private",
                "from": self.username,
                "to": recipient,
                "text": text,
                "time": datetime.now(timezone.utc).isoformat(),
            }
            if recipient == self.username:
                self.send_message(event)
                return {"ok": True, "message": "Private message sent."}, False
            if not self.server.send_to_user(recipient, event):
                return {"ok": False, "error": f"{recipient} is not online."}, False
            return {"ok": True, "message": f"Private message sent to {recipient}."}, False

        if action == "logout":
            return {"ok": True, "message": "Logged out."}, True

        return {"ok": False, "error": "Unknown action."}, False

    def finish(self) -> None:
        if self.username is not None:
            self.server.remove_session(self.username, self)
        super().finish()


class ChatClient:
    def __init__(self, connection: socket.socket):
        self.connection = connection
        self.reader = connection.makefile("r", encoding="utf-8")
        self.send_lock = threading.Lock()
        self.pending_lock = threading.Lock()
        self.pending: dict[int, queue.Queue[dict[str, Any]]] = {}
        self.next_id = 1
        self.closed = threading.Event()
        self.reader_thread = threading.Thread(target=self._read_messages, daemon=True)

    def start(self) -> None:
        self.reader_thread.start()

    def _read_messages(self) -> None:
        try:
            while not self.closed.is_set():
                line = self.reader.readline(MAX_LINE_BYTES + 1)
                if not line:
                    break
                if len(line.encode("utf-8")) > MAX_LINE_BYTES:
                    break
                try:
                    message = json.loads(line)
                except json.JSONDecodeError:
                    continue
                if not isinstance(message, dict):
                    continue
                if message.get("event") == "chat":
                    self._display_event(message)
                    continue
                request_id = message.get("id")
                with self.pending_lock:
                    waiter = self.pending.get(request_id)
                if waiter is not None:
                    waiter.put(message)
                else:
                    print(message.get("message", message.get("error", message)))
        except (OSError, UnicodeError, ValueError):
            pass
        finally:
            self.closed.set()
            with self.pending_lock:
                waiting = list(self.pending.values())
            for waiter in waiting:
                waiter.put({"ok": False, "error": "Connection closed."})

    @staticmethod
    def _display_event(event: dict[str, Any]) -> None:
        timestamp = event.get("time", "")
        sender = event.get("from", "unknown")
        text = event.get("text", "")
        if event.get("kind") == "private":
            recipient = event.get("to", "")
            print(f"\n[private {timestamp}] {sender} -> {recipient}: {text}")
        else:
            print(f"\n[{timestamp}] {sender}: {text}")

    def request(self, action: str, timeout: float = 20.0, **fields: Any) -> dict[str, Any]:
        if self.closed.is_set():
            raise ConnectionError("Connection is closed.")
        with self.pending_lock:
            request_id = self.next_id
            self.next_id += 1
            waiter: queue.Queue[dict[str, Any]] = queue.Queue(maxsize=1)
            self.pending[request_id] = waiter
        payload = {"id": request_id, "action": action, **fields}
        try:
            encoded = json.dumps(payload, separators=(",", ":"), ensure_ascii=False).encode("utf-8") + b"\n"
            if len(encoded) > MAX_LINE_BYTES:
                raise ValueError("Request is too large.")
            with self.send_lock:
                self.connection.sendall(encoded)
            response = waiter.get(timeout=timeout)
            return response
        except queue.Empty as exc:
            raise TimeoutError("The server did not respond in time.") from exc
        finally:
            with self.pending_lock:
                self.pending.pop(request_id, None)

    def close(self) -> None:
        self.closed.set()
        try:
            self.connection.shutdown(socket.SHUT_RDWR)
        except OSError:
            pass
        try:
            self.connection.close()
        except OSError:
            pass
        try:
            self.reader.close()
        except OSError:
            pass


def create_user_command(args: argparse.Namespace) -> int:
    store = AccountStore(args.users)
    password = getpass.getpass("New password: ")
    confirmation = getpass.getpass("Confirm password: ")
    if password != confirmation:
        print("Passwords do not match.", file=sys.stderr)
        return 1
    try:
        store.add_user(args.username, password)
    except (ValueError, RuntimeError) as exc:
        print(str(exc), file=sys.stderr)
        return 1
    print(f"Created user {args.username!r}.")
    return 0


def serve_command(args: argparse.Namespace) -> int:
    tls_context = None
    if bool(args.cert) != bool(args.key):
        print("Both --cert and --key are required to enable TLS.", file=sys.stderr)
        return 2
    if args.cert:
        tls_context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
        tls_context.minimum_version = ssl.TLSVersion.TLSv1_2
        try:
            tls_context.load_cert_chain(args.cert, args.key)
        except (OSError, ssl.SSLError) as exc:
            print(f"Could not load TLS certificate: {exc}", file=sys.stderr)
            return 2

    try:
        store = AccountStore(args.users)
        server = ChatTCPServer((args.host, args.port), store, tls_context)
    except (OSError, RuntimeError) as exc:
        print(f"Could not start server: {exc}", file=sys.stderr)
        return 1

    protocol = "TLS" if tls_context else "unencrypted TCP"
    print(f"Chat server listening on {args.host}:{args.port} ({protocol}).")
    if tls_context is None:
        print("Warning: credentials and messages are not encrypted.")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down.")
    finally:
        server.shutdown()
        server.server_close()
    return 0


def open_client_connection(args: argparse.Namespace) -> socket.socket:
    connection = socket.create_connection((args.host, args.port), timeout=10)
    connection.settimeout(None)
    if args.ca or args.insecure_tls:
        if args.insecure_tls:
            context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
            context.check_hostname = False
            context.verify_mode = ssl.CERT_NONE
        else:
            context = ssl.create_default_context(cafile=args.ca)
        return context.wrap_socket(connection, server_hostname=args.host)
    return connection


def client_command(args: argparse.Namespace) -> int:
    username = input("Username: ").strip()
    password = getpass.getpass("Password: ")
    try:
        connection = open_client_connection(args)
    except (OSError, ssl.SSLError) as exc:
        print(f"Could not connect: {exc}", file=sys.stderr)
        return 1

    client = ChatClient(connection)
    client.start()
    try:
        login_response = client.request("login", username=username, password=password)
    except (ConnectionError, TimeoutError, ValueError) as exc:
        print(f"Login failed: {exc}", file=sys.stderr)
        client.close()
        return 1

    if not login_response.get("ok"):
        print(login_response.get("error", "Login failed."), file=sys.stderr)
        client.close()
        return 1

    print(login_response.get("message", "Logged in."))
    print("Type a message to chat. Commands: /users, /msg USER MESSAGE, /help, /quit")
    result = 0
    try:
        while not client.closed.is_set():
            try:
                line = input("> ")
            except EOFError:
                line = "/quit"
            if not line.strip():
                continue
            if line == "/help":
                print("Enter a message to send it publicly; /users lists online users.")
                print("/msg USER MESSAGE sends a private message; /quit disconnects.")
                continue
            if line == "/users":
                response = client.request("users")
                if response.get("ok"):
                    users = response.get("users", [])
                    print("Online: " + (", ".join(users) if users else "(none)"))
                else:
                    print(response.get("error", "Request failed."), file=sys.stderr)
                continue
            if line.startswith("/msg "):
                try:
                    parts = shlex.split(line)
                except ValueError as exc:
                    print(f"Invalid command: {exc}")
                    continue
                if len(parts) < 3:
                    print("Usage: /msg USER MESSAGE")
                    continue
                response = client.request("dm", to=parts[1], text=" ".join(parts[2:]))
            elif line == "/quit":
                try:
                    response = client.request("logout", timeout=3)
                    if response.get("message"):
                        print(response["message"])
                except (ConnectionError, TimeoutError, ValueError):
                    pass
                break
            else:
                response = client.request("say", text=line)

            if not response.get("ok"):
                print(response.get("error", "Request failed."), file=sys.stderr)
            elif response.get("message"):
                print(response["message"])
    except (ConnectionError, TimeoutError, ValueError, OSError) as exc:
        print(f"Disconnected: {exc}", file=sys.stderr)
        result = 1
    finally:
        client.close()
    return result


def build_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Filesystem-authenticated TCP chat server and client.")
    subparsers = parser.add_subparsers(dest="command", required=True)

    server_parser = subparsers.add_parser("server", help="Run the chat server.")
    server_parser.add_argument("--host", default="127.0.0.1")
    server_parser.add_argument("--port", type=int, default=8765)
    server_parser.add_argument("--users", default="users.json", help="Filesystem account database.")
    server_parser.add_argument("--cert", help="TLS certificate file; requires --key.")
    server_parser.add_argument("--key", help="TLS private key file; requires --cert.")
    server_parser.set_defaults(func=serve_command)

    client_parser = subparsers.add_parser("client", help="Connect to a chat server.")
    client_parser.add_argument("--host", default="127.0.0.1")
    client_parser.add_argument("--port", type=int, default=8765)
    client_parser.add_argument("--ca", help="CA certificate file; enables verified TLS.")
    client_parser.add_argument(
        "--insecure-tls",
        action="store_true",
        help="Enable TLS without certificate verification (not recommended).",
    )
    client_parser.set_defaults(func=client_command)

    user_parser = subparsers.add_parser("create-user", help="Add an account to the filesystem database.")
    user_parser.add_argument("username")
    user_parser.add_argument("--users", default="users.json", help="Filesystem account database.")
    user_parser.set_defaults(func=create_user_command)

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_argument_parser()
    args = parser.parse_args(argv)
    if args.command == "client" and args.ca and args.insecure_tls:
        parser.error("--ca and --insecure-tls cannot be used together")
    try:
        return args.func(args)
    except KeyboardInterrupt:
        return 130


if __name__ == "__main__":
    raise SystemExit(main())