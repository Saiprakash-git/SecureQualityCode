import re
import sys

def validate_admin_password(password, username="admin"):
    if not isinstance(password, str):
        return False, ["Password must be a string"]

    errors = []

    if len(password) == 0:
        errors.append("Password cannot be empty")
        return False, errors

    if len(password) < 12:
        errors.append("Password must be at least 12 characters long")

    if len(password) > 128:
        errors.append("Password must not exceed 128 characters")

    if not re.search(r'[A-Z]', password):
        errors.append("Password must contain at least one uppercase letter")

    if not re.search(r'[a-z]', password):
        errors.append("Password must contain at least one lowercase letter")

    if not re.search(r'[0-9]', password):
        errors.append("Password must contain at least one digit")

    special_chars = "!@#$%^&*()-_=+[]{}|;:'\",.<>/?\\`~"
    if not any(c in special_chars for c in password):
        errors.append("Password must contain at least one special character")

    if username and username.lower() in password.lower():
        errors.append("Password must not contain the username")

    common_passwords = {
        "admin", "password", "123456", "admin123", "root", "toor",
        "password123", "admin@123", "admin123!", "p@ssw0rd", "changeme",
        "letmein", "welcome", "monkey", "dragon", "master", "qwerty",
        "administrator", "superuser", "admin1234", "admin@1234"
    }
    if password.lower() in common_passwords:
        errors.append("Password is too common and easily guessable")

    if re.search(r'(.)\1{2,}', password):
        errors.append("Password must not contain more than 2 consecutive identical characters")

    sequences = [
        "0123456789", "abcdefghijklmnopqrstuvwxyz",
        "qwertyuiop", "asdfghjkl", "zxcvbnm"
    ]
    lower_pwd = password.lower()
    has_sequence = False
    for seq in sequences:
        for i in range(len(seq) - 2):
            substring = seq[i:i+3]
            if substring in lower_pwd or substring[::-1] in lower_pwd:
                has_sequence = True
                break
        if has_sequence:
            break
    if has_sequence:
        errors.append("Password must not contain sequential characters")

    is_valid = len(errors) == 0
    return is_valid, errors

def is_valid_admin_password(password):
    valid, _ = validate_admin_password(password)
    return valid

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python admin_password_check.py <password>")
        sys.exit(1)

    pwd = sys.argv[1]
    valid, errors = validate_admin_password(pwd)

    if valid:
        print("VALID")
        sys.exit(0)
    else:
        print("INVALID")
        for error in errors:
            print(f"  - {error}")
        sys.exit(1)