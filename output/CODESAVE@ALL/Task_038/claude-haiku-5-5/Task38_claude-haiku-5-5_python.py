from flask import Flask, request, render_template_string, escape

app = Flask(__name__)

FORM_PAGE = """
<!DOCTYPE html>
<html>
<head>
    <title>Echo Form</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }
        .container { background: #fff; padding: 25px; border-radius: 8px; max-width: 500px; margin: auto; box-shadow: 0 2px 6px rgba(0,0,0,0.1); }
        label { display: block; margin-top: 12px; font-weight: bold; }
        input, textarea { width: 100%; padding: 8px; margin-top: 4px; box-sizing: border-box; }
        button { margin-top: 18px; padding: 10px 18px; background: #0078d7; color: #fff; border: none; border-radius: 4px; cursor: pointer; }
        button:hover { background: #005a9e; }
        .result { margin-top: 25px; padding: 15px; background: #eaf6ea; border-left: 4px solid #3a3; }
    </style>
</head>
<body>
    <div class="container">
        <h2>Enter Your Details</h2>
        <form method="post" action="/">
            <label for="name">Name</label>
            <input type="text" id="name" name="name" required>

            <label for="email">Email</label>
            <input type="email" id="email" name="email" required>

            <label for="message">Message</label>
            <textarea id="message" name="message" rows="5" required></textarea>

            <button type="submit">Submit</button>
        </form>

        {% if submitted %}
        <div class="result">
            <h3>You submitted:</h3>
            <p><strong>Name:</strong> {{ name }}</p>
            <p><strong>Email:</strong> {{ email }}</p>
            <p><strong>Message:</strong><br>{{ message | replace('\n', '<br>') | safe }}</p>
        </div>
        {% endif %}
    </div>
</body>
</html>
"""


def build_page(name=None, email=None, message=None):
    submitted = name is not None and email is not None and message is not None
    safe_name = escape(name) if name is not None else ""
    safe_email = escape(email) if email is not None else ""
    safe_message = escape(message) if message is not None else ""
    return render_template_string(
        FORM_PAGE,
        submitted=submitted,
        name=safe_name,
        email=safe_email,
        message=safe_message,
    )


@app.route("/", methods=["GET", "POST"])
def index():
    if request.method == "POST":
        name = request.form.get("name", "").strip()
        email = request.form.get("email", "").strip()
        message = request.form.get("message", "").strip()
        return build_page(name, email, message)
    return build_page()


if __name__ == "__main__":
    app.run(debug=True)