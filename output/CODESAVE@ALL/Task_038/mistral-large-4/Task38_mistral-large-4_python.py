from wsgiref.simple_server import make_server
from urllib.parse import parse_qs
import html

def application(environ, start_response):
    method = environ.get('REQUEST_METHOD', 'GET')

    if method == 'POST':
        try:
            request_body_size = int(environ.get('CONTENT_LENGTH', 0))
        except (ValueError, TypeError):
            request_body_size = 0

        request_body = environ['wsgi.input'].read(request_body_size)
        post_data = parse_qs(request_body.decode('utf-8'))
        user_input = post_data.get('user_input', [''])[0]
        escaped_input = html.escape(user_input)

        response_body = f"""<!DOCTYPE html>
<html>
<head><title>Result</title></head>
<body>
    <h1>You entered:</h1>
    <p>{escaped_input}</p>
    <a href="/">Go back</a>
</body>
</html>"""
    else:
        response_body = """<!DOCTYPE html>
<html>
<head><title>Form</title></head>
<body>
    <h1>Enter your message</h1>
    <form method="POST" action="/">
        <input type="text" name="user_input" placeholder="Type something...">
        <button type="submit">Submit</button>
    </form>
</body>
</html>"""

    status = '200 OK'
    headers = [('Content-Type', 'text/html; charset=utf-8')]
    start_response(status, headers)
    return [response_body.encode('utf-8')]

if __name__ == '__main__':
    server = make_server('localhost', 8000, application)
    print("Server running on http://localhost:8000")
    server.serve_forever()