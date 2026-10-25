import base64

def encrypt(plaintext, key):
    if not key:
        raise ValueError("Key cannot be empty")
    plaintext_bytes = plaintext.encode('utf-8')
    key_bytes = key.encode('utf-8')
    encrypted_bytes = bytearray()
    for i, byte in enumerate(plaintext_bytes):
        encrypted_bytes.append(byte ^ key_bytes[i % len(key_bytes)])
    return base64.b64encode(bytes(encrypted_bytes)).decode('utf-8')

def decrypt(ciphertext, key):
    if not key:
        raise ValueError("Key cannot be empty")
    encrypted_bytes = base64.b64decode(ciphertext)
    key_bytes = key.encode('utf-8')
    decrypted_bytes = bytearray()
    for i, byte in enumerate(encrypted_bytes):
        decrypted_bytes.append(byte ^ key_bytes[i % len(key_bytes)])
    return decrypted_bytes.decode('utf-8')

if __name__ == "__main__":
    message = "Hello, World!"
    secret_key = "mysecretkey"
    encrypted = encrypt(message, secret_key)
    print(f"Original: {message}")
    print(f"Encrypted: {encrypted}")
    decrypted = decrypt(encrypted, secret_key)
    print(f"Decrypted: {decrypted}")