import hashlib
from typing import Union


def hash_password_with_salt(password: Union[str, bytes], salt: Union[str, bytes]) -> str:
    if isinstance(password, str):
        password = password.encode("utf-8")
    if isinstance(salt, str):
        salt = salt.encode("utf-8")

    return hashlib.sha256(password + salt).hexdigest()