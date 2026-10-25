#!/usr/bin/env python3
"""
Chat system with a client and a server in one file.

Usage:
  python chat.py adduser --users users.json alice
  python chat.py server  --users users.json --host 127.0.0.1 --port 5000
  python chat.py client  --host 127.0.0.1 --port 5000

Optional TLS:
  server: --certfile cert.pem --keyfile key.pem
  client: --tls [--cafile ca.pem]

Protocol: newline-delimited JSON over TCP.
  {"action": "login", "username": "...", "password": "..."}
  {"action": "msg", "text": "..."}
  {"action": "who"}
  {"action": "logout"}
Responses: {"status": "ok"|"error", "type": "...", ...}

Passwords are stored as salted PBKDF2-HMAC-SHA256 hashes, never in plain text.
"""

import argparse
import getpass
import hashlib
import hmac
import json
import os
import re
import secrets
import socket
import ssl
import sys
import tempfile
import threading

MAX_LINE = 4096
MAX_TEXT = 1000
MAX_LOGIN_ATTEMPTS = 5
PBKDF2_ITERATIONS = 200_000
USERNAME_RE = re.compile(r"^[A-Za-z0-9_]{3,32}$")
MAX_PASSWORD_LEN = 256


# --------------------------------------------------------------------------
# Password storage / authentication (filesystem)
# --------------------------------------------------------------------------

def hash_password(password, salt=None, iterations=PBKDF2_ITERATIONS):
    """Return a storable record dict for the given password."""
    if salt is None:
        salt = secrets.token_bytes(16)
    digest = hashlib.pbkdf2_hmac("sha256", password.encode("utf-8"), salt, iterations)
    return {"salt": salt.hex(), "hash": digest.hex(), "iterations": iterations}


def verify_password(password, record):
    """Constant-time verification of a password against a stored record."""
    try:
        salt = bytes.fromhex(record["salt"])
        expected = bytes.fromhex(record["hash"])
        iterations = int(record["iterations"])
    except (KeyError, ValueError, TypeError):
        return False
    actual = hashlib.pbkdf2_hmac("sha256", password.encode("utf-8"), salt, iterations)
    return hmac.compare_digest(actual, expected)


def load_users(path):
    """Load the user database; returns {} if the file does not exist."""
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
    except FileNotFoundError:
        return {}
    if not isinstance(data, dict):
        raise ValueError("Invalid user database format")
    return data


def save_users(path, users):
    """Atomically write the user database with restrictive permissions."""
    directory = os.path.dirname(os.path.abspath(path))
    fd, tmp_path = tempfile.mkstemp(dir=directory, prefix=".users-")
    try:
        with os.fdopen(fd, "w", encoding="utf-8") as f:
            json.dump(users, f, indent=2)
            f.flush()
            os.fsync(f.fileno())
        os.chmod(tmp_path, 0o600)
        os.replace(tmp_path, path)
    except Exception:
        if os.path.exists(tmp_path):
            os.unlink(tmp_path)
        raise


def validate_username(username):
    return isinstance(username, str) and USERNAME_RE.match(username) is not None


def add_user(path, username, password):
    """Create a user in the database. Returns True on success."""
    if not validate_username(username):
        raise ValueError("Username must be 3-32 chars: letters, digits, underscore")
    if not isinstance(password, str) or not (8 <= len(password) <= MAX_PASSWORD_LEN):
        raise ValueError("Password must be 8-%d characters" % MAX_PASSWORD_LEN)
    users = load_users(path)
    if username in users:
        raise ValueError("User already exists")
    users[username] = hash_password(password)
    save_users(path, users)
    return True


_DUMMY_RECORD = hash_password("dummy-password-for-timing", salt=b"\x00" * 16)


def authenticate(users, username, password):
    """Check credentials against loaded users. Uniform work for unknown users."""
    if not validate_username(username) or not isinstance(password, str) \
            or len(password) > MAX_PASSWORD_LEN:
        verify_password("x", _DUMMY_RECORD)
        return False
    record = users.get(username)
    if record is None:
        verify_password(password, _DUMMY_RECORD)
        return False
    return verify_password(password, record)


# --------------------------------------------------------------------------
# Protocol helpers
# --------------------------------------------------------------------------

def encode_message(obj):
    """Serialize a dict to a newline-terminated JSON line."""
    return json.dumps(obj, separators=(",", ":")).encode("utf-8") + b"\n"


def parse_message(line):
    """Parse a raw line into a request dict; raises ValueError if invalid."""
    if len(line) > MAX_LINE:
        raise ValueError("Message too large")
    try:
        obj = json.loads(line.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError):
        raise ValueError("Malformed message")
    if not isinstance(obj, dict) or not isinstance(obj.get("action"), str):
        raise ValueError("Missing action")
    return obj


def make_response(status, rtype, **fields):
    resp = {"status": status, "type": rtype}
    resp.update(fields)
    return resp


# --------------------------------------------------------------------------
# Server
# --------------------------------------------------------------------------

class ChatServer:
    def __init__(self, users_path, host="127.0.0.1", port=5000, ssl_context=None):
        self.users_path = users_path
        self.host = host
        self.port = port
        self.ssl_context = ssl_context
        self.clients = {}  # username -> (wfile, write_lock)
        self.clients_lock = threading.Lock()
        self._sock = None

    def send(self, wfile, wlock, obj):
        data = encode_message(obj)
        with wlock:
            wfile.write(data)
            wfile.flush()

    def broadcast(self, obj, exclude=None):
        with self.clients_lock:
            targets = [(n, c) for n, c in self.clients.items() if n != exclude]
        for _, (wfile, wlock) in targets:
            try:
                self.send(wfile, wlock, obj)
            except (OSError, ValueError):
                pass

    def process_request(self, session, request):
        """Execute the request; returns (response_dict, close_connection)."""
        action = request.get("action")

        if action == "login":
            if session["username"]:
                return make_response("error", "login", message="Already logged in"), False
            session["attempts"] += 1
            username = request.get("username")
            password = request.get("password")
            try:
                users = load_users(self.users_path)
            except (OSError, ValueError):
                return make_response("error", "login", message="Server error"), True
            if authenticate(users, username, password):
                with self.clients_lock:
                    if username in self.clients:
                        return make_response("error", "login",
                                             message="User already connected"), False
                    self.clients[username] = (session["wfile"], session["wlock"])
                session["username"] = username
                self.broadcast(make_response("ok", "notice",
                                             message="%s joined" % username),
                               exclude=username)
                return make_response("ok", "login", message="Welcome, %s" % username), False
            close = session["attempts"] >= MAX_LOGIN_ATTEMPTS
            return make_response("error", "login",
                                 message="Invalid username or password"), close

        if not session["username"]:
            return make_response("error", "auth", message="Login required"), False

        if action == "msg":
            text = request.get("text")
            if not isinstance(text, str) or not text.strip() or len(text) > MAX_TEXT:
                return make_response("error", "msg", message="Invalid message"), False
            text = "".join(ch for ch in text if ch.isprintable())
            self.broadcast(make_response("ok", "message",
                                         sender=session["username"], text=text),
                           exclude=session["username"])
            return make_response("ok", "msg", message="Sent"), False

        if action == "who":
            with self.clients_lock:
                names = sorted(self.clients)
            return make_response("ok", "who", users=names), False

        if action == "logout":
            return make_response("ok", "logout", message="Goodbye"), True

        return make_response("error", "unknown", message="Unknown action"), False

    def handle_connection(self, conn, addr):
        session = {"username": None, "attempts": 0, "wfile": None, "wlock": threading.Lock()}
        rfile = wfile = None
        try:
            conn.settimeout(600)
            rfile = conn.makefile("rb")
            wfile = conn.makefile("wb")
            session["wfile"] = wfile
            while True:
                line = rfile.readline(MAX_LINE + 1)
                if not line:
                    break
                if len(line) > MAX_LINE:
                    self.send(wfile, session["wlock"],
                              make_response("error", "protocol", message="Message too large"))
                    break
                try:
                    request = parse_message(line)
                except ValueError as exc:
                    self.send(wfile, session["wlock"],
                              make_response("error", "protocol", message=str(exc)))
                    continue
                response, close = self.process_request(session, request)
                self.send(wfile, session["wlock"], response)
                if close:
                    break
        except (OSError, ValueError):
            pass
        finally:
            username = session["username"]
            if username:
                with self.clients_lock:
                    self.clients.pop(username, None)
                self.broadcast(make_response("ok", "notice", message="%s left" % username))
            for f in (rfile, wfile):
                try:
                    if f:
                        f.close()
                except OSError:
                    pass
            try:
                conn.close()
            except OSError:
                pass

    def serve_forever(self):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self._sock.bind((self.host, self.port))
        self._sock.listen(50)
        print("Server listening on %s:%d" % (self.host, self.port))
        try:
            while True:
                conn, addr = self._sock.accept()
                if self.ssl_context:
                    try:
                        conn = self.ssl_context.wrap_socket(conn, server_side=True)
                    except (ssl.SSLError, OSError):
                        conn.close()
                        continue
                threading.Thread(target=self.handle_connection, args=(conn, addr),
                                 daemon=True).start()
        except KeyboardInterrupt:
            print("Shutting down")
        finally:
            self._sock.close()


def build_server_ssl_context(certfile, keyfile):
    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    ctx.minimum_version = ssl.TLSVersion.TLSv1_2
    ctx.load_cert_chain(certfile, keyfile)
    return ctx


# --------------------------------------------------------------------------
# Client
# --------------------------------------------------------------------------

def prompt_credentials(input_fn=input, getpass_fn=getpass.getpass):
    """Prompt the user for a username and password (password not echoed)."""
    username = input_fn("Username: ").strip()
    password = getpass_fn("Password: ")
    return username, password


def build_login_request(username, password):
    """Construct the login request dict."""
    return {"action": "login", "username": username, "password": password}


def build_message_request(text):
    return {"action": "msg", "text": text}


def connect_to_server(host, port, use_tls=False, cafile=None):
    sock = socket.create_connection((host, port), timeout=15)
    if use_tls:
        ctx = ssl.create_default_context(cafile=cafile)
        sock = ctx.wrap_socket(sock, server_hostname=host)
    sock.settimeout(None)
    return sock


def send_request(wfile, request):
    """Prepare (serialize) and transmit a request."""
    wfile.write(encode_message(request))
    wfile.flush()


def receive_response(rfile):
    """Read one response; returns dict or None if connection closed."""
    line = rfile.readline(MAX_LINE + 1)
    if not line:
        return None
    return parse_message_response(line)


def parse_message_response(line):
    try:
        obj = json.loads(line.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError):
        return {"status": "error", "type": "protocol", "message": "Bad server response"}
    return obj if isinstance(obj, dict) else {"status": "error", "type": "protocol"}


def format_response(resp):
    rtype = resp.get("type")
    if rtype == "message":
        return "[%s] %s" % (resp.get("sender"), resp.get("text"))
    if rtype == "notice":
        return "* %s" % resp.get("message")
    if rtype == "who":
        return "Online: %s" % ", ".join(resp.get("users", []))
    if resp.get("status") == "error":
        return "Error: %s" % resp.get("message")
    return str(resp.get("message", ""))


def receiver_loop(rfile, stop_event):
    try:
        while not stop_event.is_set():
            resp = receive_response(rfile)
            if resp is None:
                print("\nConnection closed by server.")
                break
            if resp.get("type") == "msg" and resp.get("status") == "ok":
                continue
            print(format_response(resp))
    except (OSError, ValueError):
        pass
    finally:
        stop_event.set()


def run_client(host, port, use_tls=False, cafile=None):
    sock = connect_to_server(host, port, use_tls, cafile)
    rfile = sock.makefile("rb")
    wfile = sock.makefile("wb")
    try:
        username, password = prompt_credentials()
        send_request(wfile, build_login_request(username, password))
        resp = receive_response(rfile)
        if resp is None or resp.get("status") != "ok":
            print(format_response(resp) if resp else "No response from server")
            return 1
        print(format_response(resp))
        print("Commands: /who, /quit. Anything else is sent as a message.")

        stop = threading.Event()
        threading.Thread(target=receiver_loop, args=(rfile, stop), daemon=True).start()
        while not stop.is_set():
            try:
                text = input()
            except (EOFError, KeyboardInterrupt):
                text = "/quit"
            if stop.is_set():
                break
            text = text.strip()
            if not text:
                continue
            if text == "/quit":
                send_request(wfile, {"action": "logout"})
                break
            elif text == "/who":
                send_request(wfile, {"action": "who"})
            else:
                send_request(wfile, build_message_request(text[:MAX_TEXT]))
        return 0
    except OSError as exc:
        print("Connection error: %s" % exc)
        return 1
    finally:
        try:
            sock.close()
        except OSError:
            pass


# --------------------------------------------------------------------------
# Entry point
# --------------------------------------------------------------------------

def main(argv=None):
    parser = argparse.ArgumentParser(description="Simple chat system")
    sub = parser.add_subparsers(dest="mode", required=True)

    ps = sub.add_parser("server")
    ps.add_argument("--users", default="users.json")
    ps.add_argument("--host", default="127.0.0.1")
    ps.add_argument("--port", type=int, default=5000)
    ps.add_argument("--certfile")
    ps.add_argument("--keyfile")

    pc = sub.add_parser("client")
    pc.add_argument("--host", default="127.0.0.1")
    pc.add_argument("--port", type=int, default=5000)
    pc.add_argument("--tls", action="store_true")
    pc.add_argument("--cafile")

    pa = sub.add_parser("adduser")
    pa.add_argument("--users", default="users.json")
    pa.add_argument("username")

    args = parser.parse_args(argv)

    if args.mode == "server":
        ctx = None
        if args.certfile and args.keyfile:
            ctx = build_server_ssl_context(args.certfile, args.keyfile)
        else:
            print("WARNING: running without TLS; credentials are sent unencrypted.")
        ChatServer(args.users, args.host, args.port, ctx).serve_forever()
        return 0
    if args.mode == "client":
        return run_client(args.host, args.port, args.tls, args.cafile)
    if args.mode == "adduser":
        pw = getpass.getpass("New password: ")
        if pw != getpass.getpass("Confirm password: "):
            print("Passwords do not match")
            return 1
        try:
            add_user(args.users, args.username, pw)
        except ValueError as exc:
            print("Error: %s" % exc)
            return 1
        print("User created")
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(main())