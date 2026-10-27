#!/usr/bin/env python3
import json
import sys
from http.server import BaseHTTPRequestHandler, HTTPServer


DEFAULT_CONFIG = {
    "allowed_origins": ["http://localhost:3000", "https://example.com"],
    "allowed_methods": ["GET", "POST", "PUT", "DELETE", "OPTIONS"],
    "allowed_headers": ["Content-Type", "Authorization", "X-Requested-With"],
    "exposed_headers": ["X-Request-Id"],
    "allow_credentials": False,
    "max_age": 600,
}


def normalize_origin(origin):
    if origin is None:
        return None
    return origin.strip().rstrip("/").lower()


def is_origin_allowed(origin, allowed_origins):
    norm = normalize_origin(origin)
    if not norm:
        return False
    if "*" in allowed_origins:
        return True
    return norm in {normalize_origin(o) for o in allowed_origins}


def is_method_allowed(method, allowed_methods):
    if not method:
        return False
    return method.upper() in {m.upper() for m in allowed_methods}


def requested_headers_allowed(requested, allowed_headers):
    if not requested:
        return True, []
    allowed = {h.lower() for h in allowed_headers}
    if "*" in allowed:
        return True, []
    wanted = [h.strip() for h in requested.split(",") if h.strip()]
    missing = [h for h in wanted if h.lower() not in allowed]
    return len(missing) == 0, missing


def get_header(headers, name):
    if headers is None:
        return None
    if isinstance(headers, dict):
        for k, v in headers.items():
            if k.lower() == name.lower():
                return v
        return None
    try:
        return headers.get(name)
    except Exception:
        return None


def build_cors_headers(origin, config=None, is_preflight=False,
                       request_method=None, request_headers=None):
    """
    Returns a tuple (allowed, headers_dict).
    allowed is False when the origin is present and not permitted.
    headers_dict contains CORS response headers to attach.
    """
    cfg = dict(DEFAULT_CONFIG)
    if config:
        cfg.update(config)

    result = {}

    if origin is None:
        return True, result

    if not is_origin_allowed(origin, cfg["allowed_origins"]):
        return False, result

    allowed_origins = cfg["allowed_origins"]
    if "*" in allowed_origins and not cfg["allow_credentials"]:
        result["Access-Control-Allow-Origin"] = "*"
    else:
        result["Access-Control-Allow-Origin"] = origin

    result["Vary"] = "Origin"

    if cfg["allow_credentials"]:
        result["Access-Control-Allow-Credentials"] = "true"

    if cfg["exposed_headers"] and not is_preflight:
        result["Access-Control-Expose-Headers"] = ", ".join(cfg["exposed_headers"])

    if is_preflight:
        result["Access-Control-Allow-Methods"] = ", ".join(cfg["allowed_methods"])
        if cfg["allowed_headers"]:
            result["Access-Control-Allow-Headers"] = ", ".join(cfg["allowed_headers"])
        result["Access-Control-Max-Age"] = str(int(cfg["max_age"]))

    return True, result


def evaluate_cors_request(method, headers, config=None):
    """
    Evaluates a request and returns (status_code, response_headers, is_preflight_response).
    status_code is None when the request should proceed to the normal handler.
    """
    cfg = dict(DEFAULT_CONFIG)
    if config:
        cfg.update(config)

    origin = get_header(headers, "Origin")
    method = (method or "").upper()

    is_preflight = (
        method == "OPTIONS"
        and get_header(headers, "Access-Control-Request-Method") is not None
    )

    if is_preflight:
        req_method = get_header(headers, "Access-Control-Request-Method")
        req_headers = get_header(headers, "Access-Control-Request-Headers")

        origin_ok, cors_headers = build_cors_headers(
            origin, cfg, is_preflight=True
        )
        if origin is None or not origin_ok:
            return 403, {"Vary": "Origin"}, True

        if not is_method_allowed(req_method, cfg["allowed_methods"]):
            return 405, cors_headers, True

        headers_ok, _ = requested_headers_allowed(req_headers, cfg["allowed_headers"])
        if not headers_ok:
            return 403, cors_headers, True

        return 204, cors_headers, True

    origin_ok, cors_headers = build_cors_headers(origin, cfg, is_preflight=False)
    if not origin_ok:
        return 403, {"Vary": "Origin"}, False

    if origin is not None and not is_method_allowed(method, cfg["allowed_methods"]):
        return 405, cors_headers, False

    return None, cors_headers, False


CONFIG = dict(DEFAULT_CONFIG)


def make_handler(config):
    class CORSRequestHandler(BaseHTTPRequestHandler):
        server_version = "CORSDemo/1.0"

        def _send(self, status, body=None, extra_headers=None):
            payload = b""
            if body is not None:
                payload = json.dumps(body).encode("utf-8")
            self.send_response(status)
            for k, v in (extra_headers or {}).items():
                self.send_header(k, v)
            if payload:
                self.send_header("Content-Type", "application/json")
            self.send_header("Content-Length", str(len(payload)))
            self.end_headers()
            if payload:
                self.wfile.write(payload)

        def _dispatch(self):
            status, cors_headers, is_preflight = evaluate_cors_request(
                self.command, self.headers, config
            )
            if status is not None and status != 200:
                if is_preflight:
                    self._send(status, None if status == 204 else {"error": status},
                               cors_headers)
                else:
                    self._send(status, {"error": "CORS policy rejected request"},
                               cors_headers)
                return

            if self.command == "GET" and self.path.split("?")[0] == "/api/data":
                self._send(200, {"message": "hello", "items": [1, 2, 3]},
                           cors_headers)
            else:
                self._send(404, {"error": "not found"}, cors_headers)

        def do_GET(self):
            self._dispatch()

        def do_POST(self):
            self._dispatch()

        def do_PUT(self):
            self._dispatch()

        def do_DELETE(self):
            self._dispatch()

        def do_OPTIONS(self):
            self._dispatch()

        def log_message(self, format, *args):
            sys.stderr.write("%s - %s\n" % (self.address_string(), format % args))

    return CORSRequestHandler


def run_server(host="127.0.0.1", port=8000, config=None):
    cfg = dict(DEFAULT_CONFIG)
    if config:
        cfg.update(config)
    httpd = HTTPServer((host, port), make_handler(cfg))
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        httpd.server_close()


if __name__ == "__main__":
    run_server()