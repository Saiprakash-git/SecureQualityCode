from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives import serialization


def generate_rsa_openssh_private_key(
    key_size: int = 2048,
    public_exponent: int = 65537,
    password: str | bytes | None = None,
) -> str:
    """
    Generates an RSA private key and exports it in the OpenSSH private key format.

    :param key_size: Bit length of the RSA key (default: 2048).
    :param public_exponent: Public exponent (default: 65537).
    :param password: Optional password to encrypt the private key.
    :return: The OpenSSH private key as an ASCII string.
    """
    private_key = rsa.generate_private_key(
        public_exponent=public_exponent,
        key_size=key_size,
    )

    if password is not None:
        if isinstance(password, str):
            password = password.encode("utf-8")
        encryption_algorithm = serialization.BestAvailableEncryption(password)
    else:
        encryption_algorithm = serialization.NoEncryption()

    openssh_private_bytes = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.OpenSSH,
        encryption_algorithm=encryption_algorithm,
    )

    return openssh_private_bytes.decode("utf-8")


if __name__ == "__main__":
    key = generate_rsa_openssh_private_key(key_size=2048)
    print(key)