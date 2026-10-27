import hashlib

def hash_password_with_salt(password: str, salt: str) -> str:
    combined = salt + password
    return hashlib.sha256(combined.encode('utf-8')).hexdigest()