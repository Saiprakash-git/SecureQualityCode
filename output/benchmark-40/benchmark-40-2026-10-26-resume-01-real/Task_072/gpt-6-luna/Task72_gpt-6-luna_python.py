from Crypto.Cipher import AES


STATIC_IV = b"0123456789abcdef"


def encrypt_data(data: bytes, key: bytes) -> bytes:
    """Encrypt bytes with AES-CBC and PKCS#7 padding."""
    if not isinstance(data, bytes):
        raise TypeError("data must be bytes")
    if not isinstance(key, bytes):
        raise TypeError("key must be bytes")

    block_size = AES.block_size
    padding_length = block_size - len(data) % block_size
    padded_data = data + bytes([padding_length]) * padding_length

    cipher = AES.new(key, AES.MODE_CBC, iv=STATIC_IV)
    return cipher.encrypt(padded_data)