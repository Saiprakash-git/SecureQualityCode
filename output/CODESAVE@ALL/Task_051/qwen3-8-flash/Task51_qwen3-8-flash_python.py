import base64
import hashlib


def encrypt(text, key):
    if not isinstance(text, str):
        raise TypeError("text must be a string")
    if not isinstance(key, str):
        raise TypeError("key must be a string")
    if not key:
        raise ValueError("key must not be empty")

    key_bytes = hashlib.sha256(key.encode("utf-8")).digest()
    data = text.encode("utf-8")

    encrypted = bytearray(data)
    for i, byte in enumerate(data):
        encrypted[i] = byte ^ key_bytes[i % len(key_bytes)]

    return base64.b64encode(encrypted).decode("ascii")


def decrypt(cipher_text, key):
    if not isinstance(cipher_text, str):
        raise TypeError("cipher_text must be a string")
    if not isinstance(key, str):
        raise TypeError("key must be a string")
    if not key:
        raise ValueError("key must not be empty")

    encrypted = base64.b64decode(cipher_text.encode("ascii"))
    key_bytes = hashlib.sha256(key.encode("utf-8")).digest()

    decrypted = bytearray(encrypted)
    for i, byte in enumerate(encrypted):
        decrypted[i] = byte ^ key_bytes[i % len(key_bytes)]

    return decrypted.decode("utf-8")


if __name__ == "__main__":
    import sys

    text = sys.argv[1] if len(sys.argv) > 1 else "Hello, World!"
    key = sys.argv[2] if len(sys.argv) > 2 else "secret"

    encrypted_text = encrypt(text, key)
    print(encrypted_text)