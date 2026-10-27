from cryptography.hazmat.primitives import serialization
from cryptography.hazmat.primitives.asymmetric import rsa


def generate_rsa_private_key(key_size=2048, passphrase=None):
    """Generate an RSA private key and return it in OpenSSH private-key format."""
    if key_size < 2048:
        raise ValueError("key_size must be at least 2048 bits")

    private_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=key_size,
    )

    encryption = (
        serialization.NoEncryption()
        if passphrase is None
        else serialization.BestAvailableEncryption(
            passphrase.encode("utf-8") if isinstance(passphrase, str) else passphrase
        )
    )

    return private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.OpenSSH,
        encryption_algorithm=encryption,
    ).decode("utf-8")