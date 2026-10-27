from http.server import HTTPServer, BaseHTTPRequestHandler
import json
from urllib.parse import urlparse, parse_qs

def create_cors_headers(origin='*', methods='GET, POST, OPTIONS', headers='Content-Type, Authorization'):
    return {
        'Access-Control-Allow-Origin': origin,
        'Access-Control-Allow-Methods': methods,
        'Access-Control-Allow-Headers': headers,
        'Access-Control-Max-Age': '86400'
    }

def handle_get_request(path, query_params):
    return {
        'status': 'success',
        'method': 'GET',
        'path': path,
        'query': query_params,
        'data': {'items': [1, 2, 3], 'count': 3}
    }

def handle_post_request(path, body):
    try:
        data = json.loads(body) if body else {}
    except json.JSONDecodeError:
        data = {'raw': body}
    return {
        'status': 'success',
        'method': 'POST',
        'path': path,
        'received': data
    }

def handle_options_request(path):
    return {
        'status': 'preflight',
        'path': path,
        'allowed_methods': ['GET', 'POST', 'OPTIONS']
    }

class CORSRequestHandler(BaseHTTPRequestHandler):
    def _send_json_response(self, status_code, data, cors_headers):
        self.send_response(status_code)
        for key, value in cors_headers.items():
            self.send_header(key, value)
        self.send_header('Content-Type', 'application/json')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode('utf-8'))

    def do_OPTIONS(self):
        cors_headers = create_cors_headers()
        parsed = urlparse(self.path)
        response_data = handle_options_request(parsed.path)
        self._send_json_response(200, response_data, cors_headers)

    def do_GET(self):
        cors_headers = create_cors_headers()
        parsed = urlparse(self.path)
        query_params = parse_qs(parsed.query)
        response_data = handle_get_request(parsed.path, query_params)
        self._send_json_response(200, response_data, cors_headers)

    def do_POST(self):
        cors_headers = create_cors_headers()
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length).decode('utf-8') if content_length > 0 else ''
        parsed = urlparse(self.path)
        response_data = handle_post_request(parsed.path, body)
        self._send_json_response(200, response_data, cors_headers)

    def log_message(self, format, *args):
        pass

def run_server(host='localhost', port=8000):
    server_address = (host, port)
    httpd = HTTPServer(server_address, CORSRequestHandler)
    httpd.serve_forever()

if __name__ == '__main__':
    run_server()