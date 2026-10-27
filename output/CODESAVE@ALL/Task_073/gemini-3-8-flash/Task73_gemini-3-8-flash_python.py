import re


def validate_admin_password(password: str, min_length: int = 12) -> tuple[bool, list[str]]:
    """
    Validates whether a given password meets security requirements for an admin account.

    Requirements:
    - Minimum length (default: 12 characters)
    - At least one uppercase letter
    - At least one lowercase letter
    - At least one digit
    - At least one special character
    - No whitespace characters
    - Must not contain common administrative or trivial keywords (e.g., 'admin', 'password', 'root')

    :param password: The password string to validate.
    :param min_length: The minimum required length.
    :return: A tuple of (is_valid: bool, issues: list[str]).
    """
    issues = []

    if not isinstance(password, str):
        return False, ["Password must be a string."]

    if len(password) < min_length:
        issues.append(f"Must be at least {min_length} characters long.")

    if not re.search(r"[A-Z]", password):
        issues.append("Must contain at least one uppercase letter.")

    if not re.search(r"[a-z]", password):
        issues.append("Must contain at least one lowercase letter.")

    if not re.search(r"\d", password):
        issues.append("Must contain at least one numerical digit.")

    special_chars = r"[!@#$%^&*()_+\-=\[\]{}|;':\",.<>?/~`]"
    if not re.search(special_chars, password):
        issues.append("Must contain at least one special character.")

    if re.search(r"\s", password):
        issues.append("Must not contain whitespace.")

    forbidden_substrings = ["admin", "administrator", "root", "password", "qwerty"]
    lower_password = password.lower()
    for forbidden in forbidden_substrings:
        if forbidden in lower_password:
            issues.append(f"Must not contain the forbidden keyword '{forbidden}'.")

    return len(issues) == 0, issues


def is_admin_password_valid(password: str, min_length: int = 12) -> bool:
    """
    Convenience function that returns True if the admin password is valid, False otherwise.
    """
    valid, _ = validate_admin_password(password, min_length=min_length)
    return valid


if __name__ == "__main__":
    test_cases = [
        "short",
        "Admin!2024secure",
        "P@ssw0rd!2024",
        "Valid#Super9876Key",
        "Alllowercase123!@#",
        "ALLUPPERCASE123!@#",
        "NoSpecialChar12345",
        "Has Space 123!@#A",
        "Valid&Complex9876Key!",
    ]

    for test in test_cases:
        is_valid, errors = validate_admin_password(test)
        print(f"Password: {test}")
        print(f"Valid: {is_valid}")
        if not is_valid:
            print("Issues:")
            for err in errors:
                print(f"  - {err}")
        print("-" * 40)