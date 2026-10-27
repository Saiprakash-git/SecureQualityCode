import hashlib
import hmac
import os


def generate_salt(length=16):
    return os.urandom(length)


def hash_password(password, salt, iterations=200000):
    if isinstance(password, str):
        password = password.encode("utf-8")
    if isinstance(salt, str):
        salt = salt.encode("utf-8")
    digest = hashlib.pbkdf2_hmac("sha256", password, salt, iterations)
    return digest.hex()


def verify_password(password, salt, expected_hash, iterations=200000):
    computed = hash_password(password, salt, iterations)
    return hmac.compare_digest(computed, expected_hash)


def main():
    password = "example_password"
    salt = generate_salt()
    hashed = hash_password(password, salt)
    print("Salt:", salt.hex())
    print("Hash:", hashed)
    print("Verified:", verify_password(password, salt, hashed))
    return hashed


if __name__ == "__main__":
    main()