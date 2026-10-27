import html
from urllib.parse import parse_qs
from wsgiref.simple_server import make_server


def render_form():
    return """
<!doctype html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>User Input Form</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 40px;
            background: #f4f4f4;
        }
        .container {
            max-width: 600px;
            margin: 0 auto;
            background: white;
            padding: 24px;
            border-radius: 8px;
            box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
        }
        h1 {
            margin-top: 0;
        }
        label {
            display: block;
            margin-top: 16px;
            font-weight: bold;
        }
        input, textarea {
            width: 100%;
            padding: 10px;
            margin-top: 6px;
            border: 1px solid #ccc;
            border-radius: 4px;
            box-sizing: border-box;
        }
        button {
            margin-top: 20px;
            padding: 10px 16px;
            background: #2563eb;
            color: white;
            border: none;
            border-radius: 4px;
            cursor: pointer;
        }
        button:hover {
            background: #1d4ed8;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Enter Your Information</h1>
        <form action="/" method="post">
            <label for="name">Name</label>
            <input type="text" id="name" name="name" placeholder="Enter your name">

            <label for="message">Message</label>
            <textarea id="message" name="message" rows="5" placeholder="Enter your message"></textarea>

            <button type="submit">Submit</button>
        </form>
    </div>
</body>
</html>
"""


def render_result(name, message):
    safe_name = html.escape(name)
    safe_message = html.escape(message)

    return f"""
<!doctype html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Submitted Information</title>
    <style>
        body {{
            font-family: Arial, sans-serif;
            margin: 40px;
            background: #f4f4f4;
        }}
        .container {{
            max-width: 600px;
            margin: 0 auto;
            background: white;
            padding: 24px;
            border-radius: 8px;
            box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
        }}
        h1 {{
            margin-top: 0;
        }}
        .field {{
            margin-top: 16px;
        }}
        .label {{
            font-weight: bold;
        }}
        .value {{
            margin-top: 6px;
            white-space: pre-wrap;
            word-break: break-word;
        }}
        a {{
            display: inline-block;
            margin-top: 20px;
            color: #2563eb;
        }}
    </style>
</head>
<body>
    <div class="container">
        <h1>Your Submitted Information</h1>

        <div class="field">
            <div class="label">Name:</div>
            <div class="value">{safe_name}</div>
        </div>

        <div class="field">
            <div class="label">Message:</div>
            <div class="value">{safe_message}</div>
        </div>

        <a href="/">Back to form</a>
    </div>
</body>
</html>
"""


def render_not_found():
    return """
<!doctype html>
<html lang="en">
<head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Not Found</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 40px;
            background: #f4f4f4;
        }
        .container {
            max-width: 600px;
            margin: 0 auto;
            background: white;
            padding: 24px;
            border-radius: 8px;
            box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>404 Not Found</h1>
        <p>The requested page was not found.</p>
        <a href="/">Go home</a>
    </div>
</body>
</html>
"""


def get_form_data(environ):
    method = environ.get("REQUEST_METHOD", "GET")

    if method == "POST":
        try:
            content_length = int(environ.get("CONTENT_LENGTH", 0))
        except (TypeError, ValueError):
            content_length = 0

        if content_length > 0:
            body = environ["wsgi.input"].read(content_length)
        else:
            body = b""

        return parse_qs(body.decode("utf-8"))

    return parse_qs(environ.get("QUERY_STRING", ""))


def has_form_data(form_data):
    return bool(form_data.get("name") or form_data.get("message"))


def make_response(status, html_body):
    body = html_body.encode("utf-8")
    headers = [
        ("Content-Type", "text/html; charset=utf-8"),
        ("Content-Length", str(len(body))),
    ]
    return status, headers, [body]


def app(environ, start_response):
    path = environ.get("PATH_INFO", "/")

    if path != "/":
        status, headers, body = make_response("404 Not Found", render_not_found())
        start_response(status, headers)
        return body

    form_data = get_form_data(environ)

    if has_form_data(form_data):
        name = form_data.get("name", [""])[0]
        message = form_data.get("message", [""])[0]
        status, headers, body = make_response("200 OK", render_result(name, message))
    else:
        status, headers, body = make_response("200 OK", render_form())

    start_response(status, headers)
    return body


def run(host, port):
    with make_server(host, port, app) as httpd:
        httpd.serve_forever()


if __name__ == "__main__":
    run("localhost", 8000)