import re


def is_valid_admin_password(password: str) -> bool:
    """Return True if the password meets admin password requirements."""
    if not isinstance(password, str):
        return False
    if len(password) < 12:
        return False
    if not re.search(r"[A-Z]", password):
        return False
    if not re.search(r"[a-z]", password):
        return False
    if not re.search(r"[0-9]", password):
        return False
    if not re.search(r"[!@#$%^&*()\-_=+\[\]{};:'\",.<>/?\\|`~]", password):
        return False
    if re.search(r"\s", password):
        return False
    return True


def get_password_errors(password: str) -> list:
    """Return a list of reasons why the password is invalid (empty if valid)."""
    errors = []
    if not isinstance(password, str):
        return ["Password must be a string."]
    if len(password) < 12:
        errors.append("Password must be at least 12 characters long.")
    if not re.search(r"[A-Z]", password):
        errors.append("Password must contain at least one uppercase letter.")
    if not re.search(r"[a-z]", password):
        errors.append("Password must contain at least one lowercase letter.")
    if not re.search(r"[0-9]", password):
        errors.append("Password must contain at least one digit.")
    if not re.search(r"[!@#$%^&*()\-_=+\[\]{};:'\",.<>/?\\|`~]", password):
        errors.append("Password must contain at least one special character.")
    if re.search(r"\s", password):
        errors.append("Password must not contain whitespace.")
    return errors


def check_admin_password(password: str) -> str:
    """Return a status message describing whether the password is valid."""
    errors = get_password_errors(password)
    if not errors:
        return "Valid admin password."
    return "Invalid admin password:\n" + "\n".join(f"- {e}" for e in errors)


if __name__ == "__main__":
    import getpass

    pwd = getpass.getpass("Enter admin password: ")
    print(check_admin_password(pwd))