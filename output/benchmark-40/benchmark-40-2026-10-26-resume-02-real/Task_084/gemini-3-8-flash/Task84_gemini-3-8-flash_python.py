import secrets


def generate_session_id(nbytes: int = 32) -> str:
    """
    Generates a cryptographically secure, random, and unique session ID.

    :param nbytes: Number of random bytes to generate prior to base64 encoding.
    :return: A URL-safe text string representing the session ID.
    """
    return secrets.token_urlsafe(nbytes)


if __name__ == "__main__":
    session_id = generate_session_id()
    print(session_id)