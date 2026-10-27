import html
from http.server import BaseHTTPRequestHandler, HTTPServer
import urllib.parse


def render_page(title: str, body_content: str) -> str:
    return f"""<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>{html.escape(title)}</title>
    <style>
        body {{
            font-family: Arial, sans-serif;
            max-width: 600px;
            margin: 40px auto;
            padding: 20px;
            line-height: 1.6;
        }}
        .form-group {{
            margin-bottom: 15px;
        }}
        label {{
            display: block;
            margin-bottom: 5px;
            font-weight: bold;
        }}
        input[type="text"], textarea {{
            width: 100%;
            padding: 8px;
            box-sizing: border-box;
            border: 1px solid #ccc;
            border-radius: 4px;
        }}
        input[type="submit"] {{
            padding: 10px 20px;
            background-color: #0066cc;
            color: #ffffff;
            border: none;
            border-radius: 4px;
            cursor: pointer;
        }}
        input[type="submit"]:hover {{
            background-color: #0052a3;
        }}
        .result-box {{
            background-color: #f4f4f4;
            border-left: 4px solid #0066cc;
            padding: 15px;
            margin-top: 20px;
        }}
        a {{
            display: inline-block;
            margin-top: 15px;
            color: #0066cc;
            text-decoration: none;
        }}
        a:hover {{
            text-decoration: underline;
        }}
    </style>
</head>
<body>
    {body_content}
</body>
</html>"""


def render_form(action_url: str) -> str:
    form_content = f"""
    <h2>User Input Form</h2>
    <form method="POST" action="{html.escape(action_url)}">
        <div class="form-group">
            <label for="username">Name:</label>
            <input type="text" id="username" name="username" required>
        </div>
        <div class="form-group">
            <label for="message">Message:</label>
            <textarea id="message" name="message" rows="4" required></textarea>
        </div>
        <input type="submit" value="Submit">
    </form>
    """
    return render_page("Input Form", form_content)


def render_result(name: str, message: str, return_url: str) -> str:
    result_content = f"""
    <h2>Submitted Information</h2>
    <div class="result-box">
        <p><strong>Name:</strong> {html.escape(name)}</p>
        <p><strong>Message:</strong> {html.escape(message)}</p>
    </div>
    <a href="{html.escape(return_url)}">&larr; Back to form</a>
    """
    return render_page("Submission Result", result_content)


def extract_form_values(body_bytes: bytes) -> tuple:
    decoded_body = body_bytes.decode("utf-8", errors="replace")
    parsed_fields = urllib.parse.parse_qs(decoded_body)
    name = parsed_fields.get("username", [""])[0]
    message = parsed_fields.get("message", [""])[0]
    return name, message


class FormRequestHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        html_response = render_form(action_url="/")
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.end_headers()
        self.wfile.write(html_response.encode("utf-8"))

    def do_POST(self):
        content_length = int(self.headers.get("Content-Length", 0))
        post_body = self.rfile.read(content_length)
        name, message = extract_form_values(post_body)
        html_response = render_result(name=name, message=message, return_url="/")
        self.send_response(200)
        self.send_header("Content-Type", "text/html; charset=utf-8")
        self.end_headers()
        self.wfile.write(html_response.encode("utf-8"))


def run_server(host: str, port: int) -> None:
    server_address = (host, port)
    httpd = HTTPServer(server_address, FormRequestHandler)
    print(f"Server running at http://{host}:{port}/")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        httpd.server_close()


if __name__ == "__main__":
    run_server("127.0.0.1", 8000)