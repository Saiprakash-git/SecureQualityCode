import base64
import hashlib
import os
import sys


def derive_keystream(key: bytes, nonce: bytes, length: int) -> bytes:
    stream = bytearray()
    counter = 0
    while len(stream) < length:
        block = hashlib.sha256(key + nonce + counter.to_bytes(8, "big")).digest()
        stream.extend(block)
        counter += 1
    return bytes(stream[:length])


def encrypt(plaintext: str, password: str) -> str:
    nonce = os.urandom(16)
    key = hashlib.sha256(password.encode("utf-8")).digest()
    data = plaintext.encode("utf-8")
    keystream = derive_keystream(key, nonce, len(data))
    cipher = bytes(p ^ k for p, k in zip(data, keystream))
    mac = hashlib.sha256(key + nonce + cipher).digest()
    return base64.b64encode(nonce + mac + cipher).decode("ascii")


def decrypt(token: str, password: str) -> str:
    raw = base64.b64decode(token.encode("ascii"))
    nonce = raw[:16]
    mac = raw[16:48]
    cipher = raw[48:]
    key = hashlib.sha256(password.encode("utf-8")).digest()
    expected = hashlib.sha256(key + nonce + cipher).digest()
    if mac != expected:
        raise ValueError("Integrity check failed: wrong password or corrupted data")
    keystream = derive_keystream(key, nonce, len(cipher))
    data = bytes(c ^ k for c, k in zip(cipher, keystream))
    return data.decode("utf-8")


def main():
    if len(sys.argv) != 4 or sys.argv[1] not in ("enc", "dec"):
        print("Usage: python script.py enc|dec <text> <password>")
        sys.exit(1)
    mode, text, password = sys.argv[1], sys.argv[2], sys.argv[3]
    if mode == "enc":
        print(encrypt(text, password))
    else:
        print(decrypt(text, password))


if __name__ == "__main__":
    main()