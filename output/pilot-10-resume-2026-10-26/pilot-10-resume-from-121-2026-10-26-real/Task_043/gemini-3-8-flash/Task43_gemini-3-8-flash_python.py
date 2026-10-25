import datetime
import hashlib
import hmac
import html
import http.cookies
import http.server
import json
import secrets
import sqlite3
import sys
import urllib.parse
from typing import Any, Dict, Optional, Tuple

DB_FILE = ":memory:"
SESSION_COOKIE_NAME = "session_token"
SESSION_DURATION_SECONDS = 1800  # 30 minutes


def init_db(conn: sqlite3.Connection) -> None:
    with conn:
        conn.execute(
            """
            CREATE TABLE IF NOT EXISTS users (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                username TEXT UNIQUE NOT NULL,
                password_hash TEXT NOT NULL,
                salt TEXT NOT NULL
            );
            """
        )
        conn.execute(
            """
            CREATE TABLE IF NOT EXISTS sessions (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                session_hash TEXT UNIQUE NOT NULL,
                user_id INTEGER NOT NULL,
                created_at TIMESTAMP NOT NULL,
                expires_at TIMESTAMP NOT NULL,
                FOREIGN KEY (user_id) REFERENCES users (id) ON DELETE CASCADE
            );
            """
        )


def hash_password(password: str, salt: Optional[str] = None) -> Tuple[str, str]:
    if not salt:
        salt = secrets.token_hex(16)
    pw_hash = hashlib.pbkdf2_hmac(
        "sha256",
        password.encode("utf-8"),
        salt.encode("utf-8"),
        iterations=100_000,
    ).hex()
    return pw_hash, salt


def verify_password(stored_hash: str, salt: str, password_attempt: str) -> bool:
    attempt_hash, _ = hash_password(password_attempt, salt)
    return hmac.compare_digest(stored_hash, attempt_hash)


def hash_session_token(token: str) -> str:
    return hashlib.sha256(token.encode("utf-8")).hexdigest()


class SessionManager:
    def __init__(self, conn: sqlite3.Connection):
        self.conn = conn

    def create_session(self, user_id: int, duration_seconds: int = SESSION_DURATION_SECONDS) -> str:
        self.cleanup_expired_sessions()
        raw_token = secrets.token_urlsafe(32)
        session_hash = hash_session_token(raw_token)
        now = datetime.datetime.now(datetime.timezone.utc)
        expires_at = now + datetime.timedelta(seconds=duration_seconds)

        with self.conn:
            self.conn.execute(
                """
                INSERT INTO sessions (session_hash, user_id, created_at, expires_at)
                VALUES (?, ?, ?, ?)
                """,
                (session_hash, user_id, now.isoformat(), expires_at.isoformat()),
            )
        return raw_token

    def validate_session(self, raw_token: Optional[str]) -> Optional[Dict[str, Any]]:
        if not raw_token:
            return None

        session_hash = hash_session_token(raw_token)
        now_str = datetime.datetime.now(datetime.timezone.utc).isoformat()

        cursor = self.conn.cursor()
        cursor.execute(
            """
            SELECT s.id, s.user_id, s.expires_at, u.username
            FROM sessions s
            JOIN users u ON s.user_id = u.id
            WHERE s.session_hash = ? AND s.expires_at > ?
            """,
            (session_hash, now_str),
        )
        row = cursor.fetchone()
        if not row:
            return None

        return {
            "session_id": row[0],
            "user_id": row[1],
            "expires_at": row[2],
            "username": row[3],
        }

    def terminate_session(self, raw_token: Optional[str]) -> bool:
        if not raw_token:
            return False

        session_hash = hash_session_token(raw_token)
        with self.conn:
            cursor = self.conn.execute(
                "DELETE FROM sessions WHERE session_hash = ?", (session_hash,)
            )
            return cursor.rowcount > 0

    def terminate_all_user_sessions(self, user_id: int) -> int:
        with self.conn:
            cursor = self.conn.execute(
                "DELETE FROM sessions WHERE user_id = ?", (user_id,)
            )
            return cursor.rowcount

    def cleanup_expired_sessions(self) -> int:
        now_str = datetime.datetime.now(datetime.timezone.utc).isoformat()
        with self.conn:
            cursor = self.conn.execute(
                "DELETE FROM sessions WHERE expires_at <= ?", (now_str,)
            )
            return cursor.rowcount


class SecureAppRequestHandler(http.server.BaseHTTPRequestHandler):
    conn: sqlite3.Connection = None  # type: ignore
    session_manager: SessionManager = None  # type: ignore

    def _parse_form_data(self) -> Dict[str, str]:
        content_length_header = self.headers.get("Content-Length")
        if not content_length_header:
            return {}
        try:
            length = int(content_length_header)
            raw_body = self.rfile.read(length).decode("utf-8")
            parsed = urllib.parse.parse_qs(raw_body, keep_blank_values=True)
            return {k: v[0] for k, v in parsed.items()}
        except Exception:
            return {}

    def _get_cookie_token(self) -> Optional[str]:
        cookie_header = self.headers.get("Cookie")
        if not cookie_header:
            return None
        cookies = http.cookies.SimpleCookie()
        try:
            cookies.load(cookie_header)
            if SESSION_COOKIE_NAME in cookies:
                return cookies[SESSION_COOKIE_NAME].value
        except Exception:
            return None
        return None

    def _build_session_cookie(self, token: str, max_age: int) -> str:
        cookie = http.cookies.SimpleCookie()
        cookie[SESSION_COOKIE_NAME] = token
        cookie[SESSION_COOKIE_NAME]["path"] = "/"
        cookie[SESSION_COOKIE_NAME]["httponly"] = True
        cookie[SESSION_COOKIE_NAME]["samesite"] = "Lax"
        cookie[SESSION_COOKIE_NAME]["max-age"] = max_age
        # In a strict HTTPS deployment, uncomment:
        # cookie[SESSION_COOKIE_NAME]["secure"] = True
        return cookie.output(header="").strip()

    def _build_clear_cookie(self) -> str:
        cookie = http.cookies.SimpleCookie()
        cookie[SESSION_COOKIE_NAME] = ""
        cookie[SESSION_COOKIE_NAME]["path"] = "/"
        cookie[SESSION_COOKIE_NAME]["httponly"] = True
        cookie[SESSION_COOKIE_NAME]["samesite"] = "Lax"
        cookie[SESSION_COOKIE_NAME]["max-age"] = 0
        return cookie.output(header="").strip()

    def _render_page(self, title: str, body_html: str, status_code: int = 200, headers: Optional[Dict[str, str]] = None) -> None:
        full_html = f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>{html.escape(title)}</title>
    <style>
        body {{ font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; max-width: 600px; margin: 40px auto; padding: 0 20px; }}
        input[type="text"], input[type="password"] {{ width: 100%; padding: 8px; margin: 8px 0; box-sizing: border-box; }}
        button {{ background-color: #0066cc; color: white; padding: 10px 15px; border: none; cursor: pointer; border-radius: 4px; }}
        button:hover {{ background-color: #0052a3; }}
        .card {{ background: #f9f9f9; padding: 20px; border-radius: 8px; border: 1px solid #ddd; margin-bottom: 20px; }}
        .error {{ color: red; }}
        .success {{ color: green; }}
    </style>
</head>
<body>
    <h1>{html.escape(title)}</h1>
    {body_html}
</body>
</html>"""
        data = full_html.encode("utf-8")
        self.send_response(status_code)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.send_header("X-Content-Type-Options", "nosniff")
        self.send_header("X-Frame-Options", "DENY")
        if headers:
            for k, v in headers.items():
                self.send_header(k, v)
        self.end_headers()
        self.wfile.write(data)

    def _redirect(self, location: str, headers: Optional[Dict[str, str]] = None) -> None:
        self.send_response(303)
        self.send_header("Location", location)
        self.send_header("Content-Length", "0")
        if headers:
            for k, v in headers.items():
                self.send_header(k, v)
        self.end_headers()

    def do_GET(self) -> None:
        parsed_url = urllib.parse.urlparse(self.path)
        token = self._get_cookie_token()
        session = self.session_manager.validate_session(token)

        if parsed_url.path == "/" or parsed_url.path == "/dashboard":
            if session:
                body = f"""
                <div class="card">
                    <p>Welcome, <strong>{html.escape(session['username'])}</strong>!</p>
                    <p>Session Expires: <code>{html.escape(session['expires_at'])}</code></p>
                    <form method="POST" action="/logout">
                        <button type="submit">Sign Out</button>
                    </form>
                </div>
                """
                self._render_page("Dashboard", body)
            else:
                body = """
                <div class="card">
                    <h2>Sign In</h2>
                    <form method="POST" action="/login">
                        <label>Username</label>
                        <input type="text" name="username" required>
                        <label>Password</label>
                        <input type="password" name="password" required>
                        <button type="submit">Sign In</button>
                    </form>
                </div>
                <div class="card">
                    <h2>Register</h2>
                    <form method="POST" action="/register">
                        <label>Username</label>
                        <input type="text" name="username" required>
                        <label>Password</label>
                        <input type="password" name="password" required>
                        <button type="submit">Register</button>
                    </form>
                </div>
                """
                self._render_page("Sign In / Register", body)
        else:
            self._render_page("Not Found", "<p>Page not found.</p>", status_code=404)

    def do_POST(self) -> None:
        parsed_url = urllib.parse.urlparse(self.path)
        form_data = self._parse_form_data()

        if parsed_url.path == "/register":
            username = form_data.get("username", "").strip()
            password = form_data.get("password", "")

            if not username or not password:
                self._render_page("Registration Error", "<p class='error'>Username and password are required.</p><a href='/'>Back</a>", status_code=400)
                return

            pw_hash, salt = hash_password(password)
            try:
                with self.conn:
                    self.conn.execute(
                        "INSERT INTO users (username, password_hash, salt) VALUES (?, ?, ?)",
                        (username, pw_hash, salt),
                    )
                self._redirect("/")
            except sqlite3.IntegrityError:
                self._render_page("Registration Error", "<p class='error'>Username already taken.</p><a href='/'>Back</a>", status_code=409)

        elif parsed_url.path == "/login":
            username = form_data.get("username", "").strip()
            password = form_data.get("password", "")

            cursor = self.conn.cursor()
            cursor.execute("SELECT id, password_hash, salt FROM users WHERE username = ?", (username,))
            user = cursor.fetchone()

            if user and verify_password(user[1], user[2], password):
                user_id = user[0]
                token = self.session_manager.create_session(user_id)
                cookie_str = self._build_session_cookie(token, SESSION_DURATION_SECONDS)
                self._redirect("/", headers={"Set-Cookie": cookie_str})
            else:
                self