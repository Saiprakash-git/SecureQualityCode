def is_valid_admin_password(password: str, username: str = "admin") -> bool:
    """Return whether a password meets the strength policy for an admin account."""
    if not isinstance(password, str) or len(password) < 12:
        return False
    if any(character.isspace() for character in password):
        return False

    if not any(character.islower() for character in password):
        return False
    if not any(character.isupper() for character in password):
        return False
    if not any(character.isdigit() for character in password):
        return False
    if not any(not character.isalnum() for character in password):
        return False

    return not username or username.casefold() not in password.casefold()