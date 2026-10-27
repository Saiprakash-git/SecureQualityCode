import hashlib
import json
import os
import secrets
import threading
import time
from dataclasses import dataclass
from http import cookies
from wsgiref.simple_server import make_server


@dataclass(frozen=True)
class Session:
    csrf_token: str
    created_at: float
    expires_at: float


class SessionStore:
    def __init__(self, lifetime_seconds=3600, max_sessions=100000):
        if lifetime_seconds <= 0:
            raise ValueError("lifetime_seconds must be positive")
        if max_sessions <= 0:
            raise ValueError("max_sessions must be positive")
        self.lifetime_seconds = lifetime_seconds
        self.max_sessions = max_sessions
        self._sessions = {}
        self._lock = threading.RLock()

    @staticmethod
    def _key(token):
        return hashlib.sha256(token.encode("utf-8")).digest()

    def create(self):
        now = time.time()
        with self._lock:
            self._remove_expired(now)
            if len(self._sessions) >= self.max_sessions:
                return None, None

            token = secrets.token_urlsafe(32)
            key = self._key(token)
            while key in self._sessions:
                token = secrets.token_urlsafe(32)
                key = self._key(token)

            session = Session(
                csrf_token=secrets.token_urlsafe(32),
                created_at=now,
                expires_at=now + self.lifetime_seconds,
            )
            self._sessions[key] = session
            return token, session

    def get(self, token):
        if not token or len(token) > 128:
            return None
        key = self._key(token)
        now = time.time()
        with self._lock:
            session = self._sessions.get(key)
            if session is None:
                return None
            if session.expires_at <= now:
                del self._sessions[key]
                return None
            return session

    def delete(self, token):
        if not token or len(token) > 128:
            return False
        key = self._key(token)
        with self._lock:
            return self._sessions.pop(key, None) is not None

    def _remove_expired(self, now):
        expired = [
            key for key, session in self._sessions.items()
            if session.expires_at <= now
        ]
        for key in expired:
            del self._sessions[key]


class SessionApplication:
    def __init__(
        self,
        lifetime_seconds=3600,
        max_sessions=100000,
        cookie_secure=True,
        cookie_name="session",
    ):
        self.store = SessionStore(lifetime_seconds, max_sessions)
        self.cookie_secure = bool(cookie_secure)
        self.cookie_name = cookie_name

    def __call__(self, environ, start_response):
        method = environ.get("REQUEST_METHOD", "GET").upper()
        path = environ.get("PATH_INFO", "/")

        try:
            token = self._read_cookie(environ)
            if path != "/session":
                return self._respond(
                    start_response, "404 Not Found", {"error": "Not found"}
                )

            if method == "POST":
                return self._create_session(start_response, token)
            if method == "GET":
                return self._get_session(start_response, token)
            if method == "DELETE":
                return self._delete_session(environ, start_response, token)

            return self._respond(
                start_response,
                "405 Method Not Allowed",
                {"error": "Method not allowed"},
                extra_headers=[("Allow", "GET, POST, DELETE")],
            )
        except Exception:
            return self._respond(
                start_response, "500 Internal Server Error",
                {"error": "Internal server error"}
            )

    def _create_session(self, start_response, previous_token):
        token, session = self.store.create()
        if token is None:
            return self._respond(
                start_response,
                "503 Service Unavailable",
                {"error": "Session capacity reached"},
            )

        if previous_token:
            self.store.delete(previous_token)

        payload = self._session_payload(session)
        return self._respond(
            start_response,
            "201 Created",
            payload,
            extra_headers=[("Set-Cookie", self._cookie_header(token))],
        )

    def _get_session(self, start_response, token):
        session = self.store.get(token)
        if session is None:
            return self._respond(
                start_response, "401 Unauthorized",
                {"error": "Session not found or expired"}
            )
        return self._respond(
            start_response, "200 OK", self._session_payload(session)
        )

    def _delete_session(self, environ, start_response, token):
        session = self.store.get(token)
        if session is None:
            return self._respond(
                start_response, "401 Unauthorized",
                {"error": "Session not found or expired"}
            )

        supplied_csrf = environ.get("HTTP_X_CSRF_TOKEN", "")
        if not secrets.compare_digest(supplied_csrf, session.csrf_token):
            return self._respond(
                start_response, "403 Forbidden",
                {"error": "Invalid CSRF token"}
            )

        self.store.delete(token)
        return self._respond(
            start_response,
            "200 OK",
            {"message": "Session terminated"},
            extra_headers=[("Set-Cookie", self._clear_cookie_header())],
        )

    @staticmethod
    def _session_payload(session):
        return {
            "created_at": int(session.created_at),
            "expires_at": int(session.expires_at),
            "csrf_token": session.csrf_token,
        }

    def _read_cookie(self, environ):
        raw_cookie = environ.get("HTTP_COOKIE", "")
        if not raw_cookie:
            return None
        parsed = cookies.SimpleCookie()
        try:
            parsed.load(raw_cookie)
        except cookies.CookieError:
            return None
        morsel = parsed.get(self.cookie_name)
        return morsel.value if morsel is not None else None

    def _cookie_header(self, token):
        parts = [
            f"{self.cookie_name}={token}",
            "Path=/",
            f"Max-Age={self.store.lifetime_seconds}",
            "HttpOnly",
            "SameSite=Strict",
        ]
        if self.cookie_secure:
            parts.append("Secure")
        return "; ".join(parts)

    def _clear_cookie_header(self):
        parts = [
            f"{self.cookie_name}=",
            "Path=/",
            "Max-Age=0",
            "HttpOnly",
            "SameSite=Strict",
        ]
        if self.cookie_secure:
            parts.append("Secure")
        return "; ".join(parts)

    @staticmethod
    def _respond(start_response, status, payload, extra_headers=None):
        body = json.dumps(payload, separators=(",", ":")).encode("utf-8")
        headers = [
            ("Content-Type", "application/json; charset=utf-8"),
            ("Content-Length", str(len(body))),
            ("Cache-Control", "no-store"),
            ("Pragma", "no-cache"),
            ("X-Content-Type-Options", "nosniff"),
            ("Referrer-Policy", "no-referrer"),
        ]
        if extra_headers:
            headers.extend(extra_headers)
        start_response(status, headers)
        return [body]


def create_app(
    lifetime_seconds=3600,
    max_sessions=100000,
    cookie_secure=True,
    cookie_name="session",
):
    return SessionApplication(
        lifetime_seconds=lifetime_seconds,
        max_sessions=max_sessions,
        cookie_secure=cookie_secure,
        cookie_name=cookie_name,
    )


app = create_app(
    lifetime_seconds=int(os.environ.get("SESSION_LIFETIME_SECONDS", "3600")),
    max_sessions=int(os.environ.get("SESSION_MAX_COUNT", "100000")),
    cookie_secure=os.environ.get("SESSION_COOKIE_SECURE", "1").lower()
    not in {"0", "false", "no"},
)


if __name__ == "__main__":
    host = os.environ.get("HOST", "127.0.0.1")
    port = int(os.environ.get("PORT", "8000"))
    with make_server(host, port, app) as server:
        server.serve_forever()