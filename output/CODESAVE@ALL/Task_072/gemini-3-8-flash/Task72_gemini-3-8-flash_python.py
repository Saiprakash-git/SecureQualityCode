from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives import padding

STATIC_IV = b"0123456789abcdef"

def pad_data(data: bytes, block_size_bits: int = 128) -> bytes:
    padder = padding.PKCS7(block_size_bits).padder()
    return padder.update(data) + padder.finalize()

def encrypt_data_aes_cbc(data: bytes, key: bytes, iv: bytes = STATIC_IV) -> bytes:
    cipher = Cipher(algorithms.AES(key), modes.CBC(iv))
    encryptor = cipher.encryptor()
    padded_data = pad_data(data, algorithms.AES.block_size)
    encrypted_data = encryptor.update(padded_data) + encryptor.finalize()
    return encrypted_data

if __name__ == "__main__":
    test_key = b"0123456789abcdef0123456789abcdef"
    test_data = b"Hello, World! This is data to encrypt."
    ciphertext = encrypt_data_aes_cbc(test_data, test_key)
    print("Ciphertext (hex):", ciphertext.hex())