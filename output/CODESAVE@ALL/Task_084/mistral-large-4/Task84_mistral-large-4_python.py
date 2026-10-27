import secrets
import string

def generate_session_id(length=32, alphabet=string.ascii_letters + string.digits):
    return ''.join(secrets.choice(alphabet) for _ in range(length))