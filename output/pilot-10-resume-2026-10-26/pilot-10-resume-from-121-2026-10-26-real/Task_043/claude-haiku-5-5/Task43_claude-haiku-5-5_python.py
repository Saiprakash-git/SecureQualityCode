import hashlib
import hmac
import http.cookies
import http.server
import json
import os
import re
import secrets
import threading
import time
import urllib.parse

COOKIE_NAME = "sid"
IDLE_TIMEOUT_SECONDS = 15 * 60
ABSOLUTE_TIMEOUT_SECONDS = 24 * 60 * 60
PBKDF2_ITERATIONS = 200_000
MAX_BODY_BYTES = 4096
USERNAME_RE = re.compile(r"^[A-Za-z0-9_.-]{3,32}$")


def _digest_token(token):
    return hashlib.sha256(token.encode("utf-8")).hexdigest()


def generate_session_token():
    return secrets.token_urlsafe(32)


def compute_csrf_token(secret_key, session_token):
    return hmac.new(secret_key, session_token.encode("utf-8"), hashlib.sha256).hexdigest()


def verify_csrf_token(secret_key, session_token, provided):
    if not session_token or not provided:
        return False
    expected = compute_csrf_token(secret_key, session_token)
    return hmac.compare_digest(expected, provided)


def hash_password(password, salt=None):
    if salt is None:
        salt = secrets.token_bytes(16)
    derived = hashlib.pbkdf2_hmac("sha256", password.encode("utf-8"), salt, PBKDF2_ITERATIONS)
    return salt, derived


class SessionStore:
    def __init__(self, idle_timeout=IDLE_TIMEOUT_SECONDS, absolute_timeout=ABSOLUTE_TIMEOUT_SECONDS, clock=time.time):
        self._idle_timeout = idle_timeout
        self._absolute_timeout = absolute_timeout
        self._clock = clock
        self._sessions = {}
        self._lock = threading.RLock()

    def create(self, user_id, username, ip="", user_agent=""):
        token = generate_session_token()
        now = self._clock()
        with self._lock:
            self._sessions[_digest_token(token)] = {
                "user_id": user_id,
                "username": username,
                "created_at": now,
                "last_seen": now,
                "ip": ip,
                "user_agent": user_agent,
            }
        return token

    def get(self, token):
        if not token:
            return None
        key = _digest_token(token)
        with self._lock:
            session = self._sessions.get(key)
            if session is None:
                return None
            now = self._clock()
            if (now - session["last_seen"] > self._idle_timeout or
                    now - session["created_at"] > self._absolute_timeout):
                del self._sessions[key]
                return None
            session["last_seen"] = now
            return dict(session)

    def destroy(self, token):
        if not token:
            return False
        with self._lock:
            return self._sessions.pop(_digest_token(token), None) is not None

    def destroy_user_sessions(self, user_id, except_token=None):
        keep = _digest_token(except_token) if except_token else None
        removed = 0
        with self._lock:
            for key in list(self._sessions.keys()):
                if self._sessions[key]["user_id"] == user_id and key != keep:
                    del self._sessions[key]
                    removed += 1
        return removed

    def purge_expired(self):
        now = self._clock()
        removed = 0
        with self._lock:
            for key in list(self._sessions.keys()):
                s = self._sessions[key]
                if (now - s["last_seen"] > self._idle_timeout or
                        now - s["created_at"] > self._absolute_timeout):
                    del self._sessions[key]
                    removed += 1
        return removed


class UserStore:
    def __init__(self):
        self._users = {}
        self._next_id = 1
        self._lock = threading.RLock()

    def register(self, username, password):
        if not isinstance(username, str) or not USERNAME_RE.match(username):
            return None, "invalid username"
        if not isinstance(password, str) or not (8 <= len(password) <= 256):
            return None, "password must be 8-256 characters"
        with self._lock:
            if username.lower() in self._users:
                return None, "username already taken"
            salt, derived = hash_password(password)
            user_id = self._next_id
            self._next_id += 1
            self._users[username.lower()] = {
                "id": user_id,
                "username": username,
                "salt": salt,
                "hash": derived,
            }
        return user_id, None

    def authenticate(self, username, password):
        if not isinstance(username, str) or not isinstance(password, str):
            return None
        with self._lock:
            record = self._users.get(username.lower()) if isinstance(username, str) else None
        if record is None:
            hash_password("dummy-password-for-timing")
            return None
        _, derived = hash_password(password, record["salt"])
        if hmac.compare_digest(derived, record["hash"]):
            return {"id": record["id"], "username": record["username"]}
        return None


def parse_session_cookie(cookie_header):
    if not cookie_header:
        return None
    try:
        cookies = http.cookies.SimpleCookie()
        cookies.load(cookie_header)
    except http.cookies.CookieError:
        return None
    morsel = cookies.get(COOKIE_NAME)
    return morsel.value if morsel else None


def build_set_cookie(token, max_age, secure):
    parts = [f"{COOKIE_NAME}={token}", "Path=/", "HttpOnly", "SameSite=Strict", f"Max-Age={max_age}"]
    if secure:
        parts.append("Secure")
    return "; ".join(parts)


class AppState:
    def __init__(self, secret_key=None, secure_cookie=False):
        self.secret_key = secret_key or secrets.token_bytes(32)
        self.secure_cookie = secure_cookie
        self.sessions = SessionStore()
        self.users = UserStore()


def make_handler(state):
    class SessionHandler(http.server.BaseHTTPRequestHandler):
        server_version = "SecureSessions/1.0"

        def log_message(self, format, *args):
            pass

        def _send_json(self, status, payload, set_cookie=None):
            body = json.dumps(payload).encode("utf-8")
            self.send_response(status)
            self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(body)))
            self.send_header("Cache-Control", "no-store")
            self.send_header("Pragma", "no-cache")
            self.send_header("X-Content-Type-Options", "nosniff")
            self.send_header("X-Frame-Options", "DENY")
            self.send_header("Referrer-Policy", "no-referrer")
            if set_cookie:
                self.send_header("Set-Cookie", set_cookie)
            self.end_headers()
            self.wfile.write(body)

        def _read_json(self):
            try:
                length = int(self.headers.get("Content-Length", "0"))
            except ValueError:
                return None
            if length < 0 or length > MAX_BODY_BYTES:
                return None
            raw = self.rfile.read(length) if length else b""
            if not raw:
                return {}
            try:
                data = json.loads(raw.decode("utf-8"))
            except (ValueError, UnicodeDecodeError):
                return None
            return data if isinstance(data, dict) else None

        def _current_session(self):
            token = parse_session_cookie(self.headers.get("Cookie"))
            session = state.sessions.get(token)
            return token, session

        def _client_ip(self):
            return self.client_address