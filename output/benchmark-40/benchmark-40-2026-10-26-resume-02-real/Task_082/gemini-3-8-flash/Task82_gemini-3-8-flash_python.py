import hashlib
import os


def generate_salt(length: int = 16) -> bytes:
    """Generate a cryptographically secure random salt."""
    return os.urandom(length)


def hash_password(
    password: str,
    salt: bytes,
    iterations: int = 600_000,
    hash_name: str = "sha256"
) -> str:
    """
    Compute and return the hex-encoded PBKDF2 HMAC hash of a password combined with a salt.

    :param password: The plain text password to hash.
    :param salt: Cryptographic salt bytes.
    :param iterations: Number of PBKDF2 iterations.
    :param hash_name: Hash algorithm to use.
    :return: Hexadecimal string representation of the computed hash.
    """
    password_bytes = password.encode("utf-8")
    derived_key = hashlib.pbkdf2_hmac(
        hash_name,
        password_bytes,
        salt,
        iterations
    )
    return derived_key.hex()


def verify_password(
    password: str,
    salt: bytes,
    expected_hash: str,
    iterations: int = 600_000,
    hash_name: str = "sha256"
) -> bool:
    """Verify that a password and salt match an expected hash."""
    computed_hash = hash_password(password, salt, iterations, hash_name)
    return hashlib.compare_digest(computed_hash, expected_hash)


if __name__ == "__main__":
    sample_password = "SecurePassword123!"
    sample_salt = generate_salt()

    password_hash = hash_password(sample_password, sample_salt)

    print(f"Salt (hex): {sample_salt.hex()}")
    print(f"Hash:       {password_hash}")
    print(f"Verified:   {verify_password(sample_password, sample_salt, password_hash)}")