import re
from typing import Callable, Iterable, Optional, Sequence, Union
from urllib.parse import urlsplit


WSGIApp = Callable
StartResponse = Callable
OriginPolicy = Union[str, Iterable[str], Callable[[str], bool]]


class CORSMiddleware:
    """Configurable CORS middleware for WSGI applications."""

    _TOKEN_RE = re.compile(r"^[!#$%&'*+\-.^_`|~0-9A-Za-z]+$")
    _CORS_HEADERS = {
        "access-control-allow-origin",
        "access-control-allow-credentials",
        "access-control-allow-methods",
        "access-control-allow-headers",
        "access-control-max-age",
        "access-control-expose-headers",
    }

    def __init__(
        self,
        app: WSGIApp,
        allowed_origins: OriginPolicy = "*",
        allowed_methods: Iterable[str] = ("GET", "POST", "PUT", "PATCH", "DELETE", "OPTIONS"),
        allowed_headers: Union[str, Iterable[str]] = "*",
        expose_headers: Iterable[str] = (),
        allow_credentials: bool = False,
        max_age: Optional[int] = 600,
    ):
        self.app = app
        self.allow_credentials = bool(allow_credentials)

        if isinstance(allowed_origins, str):
            self._origin_wildcard = allowed_origins == "*"
            self._origin_policy = None if self._origin_wildcard else {allowed_origins}
        elif callable(allowed_origins):
            self._origin_wildcard = False
            self._origin_policy = allowed_origins
        else:
            self._origin_wildcard = False
            self._origin_policy = set(allowed_origins)

        self.allowed_methods = tuple(dict.fromkeys(
            self._validate_token(method.upper(), "allowed method")
            for method in allowed_methods
        ))

        if isinstance(allowed_headers, str) and allowed_headers == "*":
            self._headers_wildcard = True
            self.allowed_headers = set()
        else:
            self._headers_wildcard = False
            values = (allowed_headers,) if isinstance(allowed_headers, str) else allowed_headers
            self.allowed_headers = {
                self._validate_token(header.lower(), "allowed header")
                for header in values
            }

        self.expose_headers = tuple(dict.fromkeys(
            self._validate_token(header, "exposed header")
            for header in expose_headers
        ))

        if max_age is not None and (not isinstance(max_age, int) or max_age < 0):
            raise ValueError("max_age must be a non-negative integer or None")
        self.max_age = max_age

    @classmethod
    def _validate_token(cls, value: str, label: str) -> str:
        if not isinstance(value, str) or not cls._TOKEN_RE.fullmatch(value):
            raise ValueError(f"Invalid {label}: {value!r}")
        return value

    @staticmethod
    def _valid_origin(origin: str) -> bool:
        if not origin or any(character in origin for character in "\r\n, \t"):
            return False
        if origin == "null":
            return True
        try:
            parsed = urlsplit(origin)
            return (
                bool(parsed.scheme)
                and bool(parsed.netloc)
                and parsed.hostname is not None
                and parsed.path == ""
                and not parsed.query
                and not parsed.fragment
                and parsed.username is None
                and parsed.password is None
                and parsed.port is not None or (
                    bool(parsed.scheme)
                    and bool(parsed.netloc)
                    and parsed.hostname is not None
                    and parsed.path == ""
                    and not parsed.query
                    and not parsed.fragment
                    and parsed.username is None
                    and parsed.password is None
                    and parsed.port is None
                )
            )
        except ValueError:
            return False

    def _origin_allowed(self, origin: str) -> bool:
        if not self._valid_origin(origin):
            return False
        if self._origin_wildcard:
            return True
        if callable(self._origin_policy):
            return bool(self._origin_policy(origin))
        return origin in self._origin_policy

    @staticmethod
    def _append_vary(headers: list, values: Iterable[str]) -> list:
        vary_index = next(
            (i for i, (name, _) in enumerate(headers) if name.lower() == "vary"),
            None,
        )
        if vary_index is None:
            headers.append(("Vary", ", ".join(values)))
            return headers

        name, current = headers[vary_index]
        existing = [item.strip() for item in current.split(",") if item.strip()]
        if "*" in existing:
            return headers
        seen = {item.lower() for item in existing}
        for value in values:
            if value.lower() not in seen:
                existing.append(value)
                seen.add(value.lower())
        headers[vary_index] = (name, ", ".join(existing))
        return headers

    def _clean_headers(self, headers: Sequence[tuple]) -> list:
        return [
            (name, value)
            for name, value in headers
            if name.lower() not in self._CORS_HEADERS
        ]

    def _actual_response_headers(self, headers: Sequence[tuple], origin: str) -> list:
        result = self._clean_headers(headers)
        if not self._origin_allowed(origin):
            return self._append_vary(result, ("Origin",))

        allow_origin = (
            origin
            if self.allow_credentials or not self._origin_wildcard
            else "*"
        )
        result.append(("Access-Control-Allow-Origin", allow_origin))
        if self.allow_credentials:
            result.append(("Access-Control-Allow-Credentials", "true"))
        if self.expose_headers:
            result.append(("Access-Control-Expose-Headers", ", ".join(self.expose_headers)))
        return self._append_vary(result, ("Origin",))

    def _preflight_response(
        self,
        environ: dict,
        start_response: StartResponse,
        origin: str,
        requested_method: str,
        requested_headers: str,
    ):
        if not self._origin_allowed(origin):
            return self._error_response(start_response, "403 Forbidden", b"CORS origin denied")

        if requested_method not in self.allowed_methods:
            return self._error_response(start_response, "403 Forbidden", b"CORS method denied")

        requested_names = []
        if requested_headers.strip():
            for item in requested_headers.split(","):
                name = item.strip()
                if not name or not self._TOKEN_RE.fullmatch(name):
                    return self._error_response(
                        start_response, "400 Bad Request", b"Invalid requested header"
                    )
                requested_names.append(name)

        if not self._headers_wildcard:
            disallowed = [
                name for name in requested_names
                if name.lower() not in self.allowed_headers
            ]
            if disallowed:
                return self._error_response(
                    start_response, "403 Forbidden", b"CORS header denied"
                )

        allow_origin = (
            origin
            if self.allow_credentials or not self._origin_wildcard
            else "*"
        )
        headers = [
            ("Access-Control-Allow-Origin", allow_origin),
            ("Access-Control-Allow-Methods", ", ".join(self.allowed_methods)),
        ]

        if requested_names:
            # Echo requested names so wildcard header policies also work with
            # credentialed requests and headers such as Authorization.
            headers.append(("Access-Control-Allow-Headers", ", ".join(requested_names)))
        elif not self._headers_wildcard and self.allowed_headers:
            headers.append((
                "Access-Control-Allow-Headers",
                ", ".join(sorted(self.allowed_headers)),
            ))

        if self.allow_credentials:
            headers.append(("Access-Control-Allow-Credentials", "true"))
        if self.max_age is not None:
            headers.append(("Access-Control-Max-Age", str(self.max_age)))

        headers = self._append_vary(
            headers,
            ("Origin", "Access-Control-Request-Method", "Access-Control-Request-Headers"),
        )
        start_response("204 No Content", headers)
        return [b""]

    @staticmethod
    def _error_response(start_response: StartResponse, status: str, body: bytes):
        start_response(status, [
            ("Content-Type", "text/plain; charset=utf-8"),
            ("Content-Length", str(len(body))),
            ("Cache-Control", "no-store"),
        ])
        return [body]

    def __call__(self, environ: dict, start_response: StartResponse):
        origin = environ.get("HTTP_ORIGIN")
        method = environ.get("REQUEST_METHOD", "GET").upper()
        requested_method = environ.get("HTTP_ACCESS_CONTROL_REQUEST_METHOD")
        if method == "OPTIONS" and origin and requested_method:
            return self._preflight_response(
                environ,
                start_response,
                origin,
                requested_method,
                environ.get("HTTP_ACCESS_CONTROL_REQUEST_HEADERS", ""),
            )

        def cors_start_response(status, headers, exc_info=None):
            response_headers = list(headers)
            if origin:
                response_headers = self._actual_response_headers(response_headers, origin)
            return start_response(status, response_headers, exc_info)

        return self.app(environ, cors_start_response)