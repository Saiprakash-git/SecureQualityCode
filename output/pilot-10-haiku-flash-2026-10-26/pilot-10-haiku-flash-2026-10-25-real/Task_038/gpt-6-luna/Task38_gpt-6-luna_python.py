from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from html import escape
from urllib.parse import parse_qs


MAX_BODY_SIZE = 1_000_000


def render_page(message="", submitted=False):
    escaped_message = escape(message, quote=True)
    result = (
        f"<section><h2>You entered:</h2><p>{escaped_message}</p></section>"
        if submitted
        else ""
    )
    return f"""<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Input Form</title>
  <style>
    body {{ font-family: sans-serif; line-height: 1.5; margin: 2rem auto; max-width: 42rem; padding: 0 1rem; }}
    textarea {{ box-sizing: border-box; display: block; margin: .5rem 0 1rem; min-height: 8rem; width: 100%; }}
    button {{ padding: .5rem 1rem; }}
    section {{ border-top: 1px solid #ccc; margin-top: 2rem; padding-top: 1rem; }}
    section p {{ overflow-wrap: anywhere; white-space: pre-wrap; }}
  </style>
</head>
<body>
  <h1>Input Form</h1>
  <form method="post" action="/">
    <label for="message">Enter some text:</label>
    <textarea id="message" name="message">{escaped_message}</textarea>
    <button type="submit">Submit</button>
  </form>
  {result}
</body>
</html>
"""


class FormHandler(BaseHTTPRequestHandler):
    def send_html(self, status, content):
        body = content.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("X-Content-Type-Options", "nosniff")
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        self.send_html(200, render_page())

    def do_POST(self):
        try:
            content_length = int(self.headers.get("Content-Length", "0"))
        except ValueError:
            self.send_html(400, render_page())
            return

        if content_length < 0:
            self.send_html(400, render_page())
            return
        if content_length > MAX_BODY_SIZE:
            self.send_html(413, render_page())
            return

        body = self.rfile.read(content_length)
        try:
            fields = parse_qs(
                body.decode("utf-8"),
                keep_blank_values=True,
                strict_parsing=False,
            )
        except UnicodeDecodeError:
            self.send_html(400, render_page())
            return

        message = fields.get("message", [""])[0]
        self.send_html(200, render_page(message, submitted=True))

    def log_message(self, format_string, *args):
        super().log_message(format_string, *args)


def run_server(host="127.0.0.1", port=8000):
    server = ThreadingHTTPServer((host, port), FormHandler)
    print(f"Serving on http://{host}:{port}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    run_server()