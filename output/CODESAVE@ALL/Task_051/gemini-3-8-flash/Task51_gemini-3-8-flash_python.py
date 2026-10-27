import base64
import hashlib
import os
import struct


def _rotl32(value: int, shift: int) -> int:
    return ((value << shift) & 0xFFFFFFFF) | (value >> (32 - shift))


def _quarter_round(state: list, a: int, b: int, c: int, d: int) -> None:
    state[a] = (state[a] + state[b]) & 0xFFFFFFFF
    state[d] = _rotl32(state[d] ^ state[a], 16)
    state[c] = (state[c] + state[d]) & 0xFFFFFFFF
    state[b] = _rotl32(state[b] ^ state[c], 12)
    state[a] = (state[a] + state[b]) & 0xFFFFFFFF
    state[d] = _rotl32(state[d] ^ state[a], 8)
    state[c] = (state[c] + state[d]) & 0xFFFFFFFF
    state[b] = _rotl32(state[b] ^ state[c], 7)


def _chacha20_block(key: bytes, counter: int, nonce: bytes) -> bytes:
    constants = [0x61707865, 0x3320646E, 0x79622D32, 0x6B206574]
    key_words = list(struct.unpack("<8I", key))
    nonce_words = list(struct.unpack("<3I", nonce))
    initial_state = constants + key_words + [counter] + nonce_words

    state = list(initial_state)
    for _ in range(10):
        _quarter_round(state, 0, 4, 8, 12)
        _quarter_round(state, 1, 5, 9, 13)
        _quarter_round(state, 2, 6, 10, 14)
        _quarter_round(state, 3, 7, 11, 15)
        _quarter_round(state, 0, 5, 10, 15)
        _quarter_round(state, 1, 6, 11, 12)
        _quarter_round(state, 2, 7, 8, 13)
        _quarter_round(state, 3, 4, 9, 14)

    block = bytearray(64)
    for i in range(16):
        val = (state[i] + initial_state[i]) & 0xFFFFFFFF
        struct.pack_into("<I", block, i * 4, val)
    return bytes(block)


def chacha20_crypt(data: bytes, key: bytes, nonce: bytes, counter: int = 1) -> bytes:
    output = bytearray(len(data))
    for i in range(0, len(data), 64):
        keystream = _chacha20_block(key, counter, nonce)
        counter = (counter + 1) & 0xFFFFFFFF
        chunk = data[i : i + 64]
        for j in range(len(chunk)):
            output[i + j] = chunk[j] ^ keystream[j]
    return bytes(output)


def derive_key(password: str, salt: bytes) -> bytes:
    return hashlib.pbkdf2_hmac("sha256", password.encode("utf-8"), salt, 100000, 32)


def encrypt(plaintext: str, password: str) -> str:
    salt = os.urandom(16)
    nonce = os.urandom(12)
    key = derive_key(password, salt)
    plaintext_bytes = plaintext.encode("utf-8")
    ciphertext = chacha20_crypt(plaintext_bytes, key, nonce)
    payload = salt + nonce + ciphertext
    return base64.b64encode(payload).decode("ascii")


def decrypt(encrypted_payload: str, password: str) -> str:
    raw = base64.b64decode(encrypted_payload.encode("ascii"))
    if len(raw) < 28:
        raise ValueError("Invalid ciphertext length.")
    salt = raw[:16]
    nonce = raw[16:28]
    ciphertext = raw[28:]
    key = derive_key(password, salt)
    decrypted_bytes = chacha20_crypt(ciphertext, key, nonce)
    return decrypted_bytes.decode("utf-8")


if __name__ == "__main__":
    message = "Confidential Data: Encrypting with ChaCha20."
    secret_key = "MySecurePassphrase!2024"

    encrypted_text = encrypt(message, secret_key)
    print("Ciphertext (Base64):", encrypted_text)

    decrypted_text = decrypt(encrypted_text, secret_key)
    print("Decrypted Message:", decrypted_text)