import json
from dataclasses import dataclass, field
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, HTTPServer
from typing import Dict, List, Optional, Set, Tuple
from urllib.parse import urlparse


@dataclass
class CORSConfig:
    allowed_origins: Set[str] = field(default_factory=lambda: {"*"})
    allowed_methods: Set[str] = field(
        default_factory=lambda: {"GET", "POST", "PUT", "DELETE", "OPTIONS", "PATCH"}
    )
    allowed_headers: Set[str] = field(
        default_factory=lambda: {"Content-Type", "Authorization", "X-Requested-With"}
    )
    exposed_headers: Set[str] = field(default_factory=lambda: {"X-Total-Count", "X-Request-ID"})
    allow_credentials: bool = False
    max_age: int = 86400

    def is_origin_allowed(self, origin: str) -> bool:
        if not origin:
            return False
        if "*" in self.allowed_origins:
            return True
        return origin in self.allowed_origins


class CORSService:
    def __init__(self, config: CORSConfig):
        self.config = config

    def handle_preflight(self, origin: Optional[str], request_method: Optional[str], request_headers: Optional[str]) -> Tuple[int, Dict[str, str], bytes]:
        if not origin or not self.config.is_origin_allowed(origin):
            return HTTPStatus.FORBIDDEN, {"Content-Type": "text/plain"}, b"Disallowed CORS origin"

        if not request_method or request_method.upper() not in self.config.allowed_methods:
            return HTTPStatus.METHOD_NOT_ALLOWED, {"Content-Type": "text/plain"}, b"Method not allowed"

        if request_headers:
            requested = {h.strip().lower() for h in request_headers.split(",")}
            allowed = {h.lower() for h in self.config.allowed_headers}
            if not requested.issubset(allowed):
                return HTTPStatus.FORBIDDEN, {"Content-Type": "text/plain"}, b"Header not allowed"

        headers: Dict[str, str] = {}
        if "*" in self.config.allowed_origins and not self.config.allow_credentials:
            headers["Access-Control-Allow-Origin"] = "*"
        else:
            headers["Access-Control-Allow-Origin"] = origin
            headers["Vary"] = "Origin"

        if self.config.allow_credentials:
            headers["Access-Control-Allow-Credentials"] = "true"

        headers["Access-Control-Allow-Methods"] = ", ".join(sorted(self.config.allowed_methods))
        headers["Access-Control-Allow-Headers"] = ", ".join(sorted(self.config.allowed_headers))
        headers["Access-Control-Max-Age"] = str(self.config.max_age)

        return HTTPStatus.NO_CONTENT, headers, b""

    def apply_actual_request_cors(self, origin: Optional[str], response_headers: Dict[str, str]) -> Dict[str, str]:
        if not origin or not self.config.is_origin_allowed(origin):
            return response_headers

        headers = dict(response_headers)
        if "*" in self.config.allowed_origins and not self.config.allow_credentials:
            headers["Access-Control-Allow-Origin"] = "*"
        else:
            headers["Access-Control-Allow-Origin"] = origin
            vary = headers.get("Vary")
            headers["Vary"] = f"{vary}, Origin" if vary else "Origin"

        if self.config.allow_credentials:
            headers["Access-Control-Allow-Credentials"] = "true"

        if self.config.exposed_headers:
            headers["Access-Control-Expose-Headers"] = ", ".join(sorted(self.config.exposed_headers))

        return headers


class CORSRequestHandler(BaseHTTPRequestHandler):
    cors_config = CORSConfig(
        allowed_origins={"http://localhost:3000", "https://example.com"},
        allowed_methods={"GET", "POST", "PUT", "DELETE", "OPTIONS"},
        allowed_headers={"Content-Type", "Authorization", "X-Api-Key"},
        exposed_headers={"X-Resource-ID"},
        allow_credentials=True,
        max_age=3600
    )
    cors_service = CORSService(cors_config)

    def do_OPTIONS(self) -> None:
        origin = self.headers.get("Origin")
        req_method = self.headers.get("Access-Control-Request-Method")
        req_headers = self.headers.get("Access-Control-Request-Headers")

        status, headers, body = self.cors_service.handle_preflight(origin, req_method, req_headers)
        self.send_response(status)
        for key, value in headers.items():
            self.send_header(key, value)
        self.end_headers()
        if body:
            self.wfile.write(body)

    def do_GET(self) -> None:
        origin = self.headers.get("Origin")
        parsed_path = urlparse(self.path)

        if parsed_path.path == "/api/items":
            payload = json.dumps({"items": ["item1", "item2", "item3"]}).encode("utf-8")
            base_headers = {
                "Content-Type": "application/json",
                "X-Resource-ID": "batch-101",
                "Content-Length": str(len(payload))
            }
            final_headers = self.cors_service.apply_actual_request_cors(origin, base_headers)

            self.send_response(HTTPStatus.OK)
            for k, v in final_headers.items():
                self.send_header(k, v)
            self.end_headers()
            self.wfile.write(payload)
        else:
            self.send_error(HTTPStatus.NOT_FOUND, "Resource not found")

    def do_POST(self) -> None:
        origin = self.headers.get("Origin")
        parsed_path = urlparse(self.path)

        if parsed_path.path == "/api/items":
            content_length = int(self.headers.get("Content-Length", 0))
            raw_body = self.rfile.read(content_length)
            
            try:
                data = json.loads(raw_body.decode("utf-8")) if raw_body else {}
            except json.JSONDecodeError:
                self.send_error(HTTPStatus.BAD_REQUEST, "Invalid JSON")
                return

            response_data = {"status": "created", "item": data}
            payload = json.dumps(response_data).encode("utf-8")
            base_headers = {
                "Content-Type": "application/json",
                "X-Resource-ID": "item-999",
                "Content-Length": str(len(payload))
            }
            final_headers = self.cors_service.apply_actual_request_cors(origin, base_headers)

            self.send_response(HTTPStatus.CREATED)
            for k, v in final_headers.items():
                self.send_header(k, v)
            self.end_headers()
            self.wfile.write(payload)
        else:
            self.send_error(HTTPStatus.NOT_FOUND, "Resource not found")


def simulate_request(service: CORSService, method: str, path: str, headers: Dict[str, str], body: bytes = b"") -> Tuple[int, Dict[str, str], bytes]:
    origin = headers.get("Origin")
    if method == "OPTIONS":
        return service.handle_preflight(
            origin,
            headers.get("Access-Control-Request-Method"),
            headers.get("Access-Control-Request-Headers")
        )

    if path == "/api/items" and method == "GET":
        payload = json.dumps({"items": ["item1", "item2"]}).encode("utf-8")
        resp_headers = {
            "Content-Type": "application/json",
            "X-Resource-ID": "batch-101"
        }
        resp_headers = service.apply_actual_request_cors(origin, resp_headers)
        return HTTPStatus.OK, resp_headers, payload

    if path == "/api/items" and method == "POST":
        payload = json.dumps({"status": "created"}).encode("utf-8")
        resp_headers = {
            "Content-Type": "application/json",
            "X-Resource-ID": "item-999"
        }
        resp_headers = service.apply_actual_request_cors(origin, resp_headers)
        return HTTPStatus.CREATED, resp_headers, payload

    return HTTPStatus.NOT_FOUND, {"Content-Type": "text/plain"}, b"Not Found"


def main() -> None:
    config = CORSConfig(
        allowed_origins={"https://trusted-client.com"},
        allowed_methods={"GET", "POST", "OPTIONS"},
        allowed_headers={"Content-Type", "Authorization"},
        exposed_headers={"X-Resource-ID"},
        allow_credentials=True,
        max_age=3600
    )
    cors = CORSService(config)

    # 1. Allowed Preflight
    status, hdrs, _ = simulate_request(cors, "OPTIONS", "/api/items", {
        "Origin": "https://trusted-client.com",
        "Access-Control-Request-Method": "POST",
        "Access-Control-Request-Headers": "Authorization, Content-Type"
    })
    assert status == HTTPStatus.NO_CONTENT
    assert hdrs.get("Access-Control-Allow-Origin") == "https://trusted-client.com"
    assert hdrs.get("Access-Control-Allow-Credentials") == "true"
    assert "POST" in hdrs.get("Access-Control-Allow-Methods", "")

    # 2. Disallowed Origin Preflight
    status, _, _ = simulate_request(cors, "OPTIONS", "/api/items", {
        "Origin": "https://untrusted-client.com",
        "Access-Control-Request-Method": "POST"
    })
    assert status == HTTPStatus.FORBIDDEN

    # 3. Disallowed Header Preflight
    status, _, _ = simulate_request(cors, "OPTIONS", "/api/items", {
        "Origin": "https://trusted-client.com",
        "Access-Control-Request-Method": "POST",
        "Access-Control-Request-Headers": "X-Malicious-Header"
    })
    assert status == HTTPStatus.FORBIDDEN

    # 4. Actual GET request with CORS
    status, hdrs, body = simulate_request(cors, "GET", "/api/items", {
        "Origin": "https://trusted-client.com"
    })
    assert status == HTTPStatus.OK
    assert hdrs.get("Access-Control-Allow-Origin") == "https://trusted-client.com"
    assert hdrs.get("Access-Control-Expose-Headers") == "X-Resource-ID"
    assert json.loads(body.decode("utf-8")) == {"items": ["item1", "item2"]}

    # 5. Non-CORS request (no Origin header)
    status, hdrs, body = simulate_request(cors, "GET", "/api/items", {})
    assert status == HTTPStatus.OK
    assert "Access-Control-Allow-Origin" not in hdrs

    print("All CORS test simulations passed successfully.")


if __name__ == "__main__":
    main()