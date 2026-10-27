import sys
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes

STATIC_IV = b"0123456789ABCDEF"
AES_BLOCK_SIZE = algorithms.AES.block_size // 8


def create_static_iv() -> bytes:
    return STATIC_IV


def initialize_cipher(key: bytes, iv: bytes) -> Cipher:
    return Cipher(algorithms.AES(key), modes.CBC(iv))


def pad_data(data: bytes, block_size: int = AES_BLOCK_SIZE) -> bytes:
    padding_length = block_size - (len(data) % block_size)
    return data + bytes([padding_length]) * padding_length


def encrypt_data(data: bytes, key: bytes) -> bytes:
    iv = create_static_iv()
    cipher = initialize_cipher(key, iv)
    padded = pad_data(data, AES_BLOCK_SIZE)
    encryptor = cipher.encryptor()
    return encryptor.update(padded) + encryptor.finalize()


if __name__ == "__main__":
    key = b"0" * 32
    plaintext = sys.argv[1].encode() if len(sys.argv) > 1 else b"Hello, World!"
    encrypted = encrypt_data(plaintext, key)
    print(encrypted.hex())