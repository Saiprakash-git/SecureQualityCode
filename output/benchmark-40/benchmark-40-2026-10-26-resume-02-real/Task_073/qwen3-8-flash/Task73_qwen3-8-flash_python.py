import re


def is_valid_admin_password(password, username="admin"):
    if not isinstance(password, str):
        return False

    if not isinstance(username, str):
        username = ""

    if len(password) < 12 or len(password) > 128:
        return False

    if re.search(r"\s", password):
        return False

    if not re.search(r"[A-Z]", password):
        return False

    if not re.search(r"[a-z]", password):
        return False

    if not re.search(r"[0-9]", password):
        return False

    if not re.search(r"[!@#$%^&*()\-_=+\[\]{}|;:,.<>?]", password):
        return False

    if re.search(r"(.)\1{3,}", password):
        return False

    lowered = password.lower()

    if username and username.lower() in lowered:
        return False

    forbidden = (
        "admin",
        "administrator",
        "root",
        "password",
        "passw0rd",
        "p@ssw0rd",
        "1234",
        "abcd",
        "qwer",
        "asdf",
        "zxcv",
        "letmein",
        "welcome",
        "iloveyou",
        "monkey",
        "dragon",
    )

    if any(item in lowered for item in forbidden):
        return False

    return True