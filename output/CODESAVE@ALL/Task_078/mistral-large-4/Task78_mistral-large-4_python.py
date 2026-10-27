from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives import serialization

def generate_rsa_openssh_private_key(key_size=2048, passphrase=None):
    private_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=key_size
    )

    if passphrase is None:
        encryption_algorithm = serialization.NoEncryption()
    else:
        if isinstance(passphrase, str):
            passphrase = passphrase.encode('utf-8')
        encryption_algorithm = serialization.BestAvailableEncryption(passphrase)

    pem_bytes = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.OpenSSH,
        encryption_algorithm=encryption_algorithm
    )

    return pem_bytes

def generate_rsa_openssh_private_key_string(key_size=2048, passphrase=None):
    pem_bytes = generate_rsa_openssh_private_key(key_size, passphrase)
    return pem_bytes.decode('utf-8')

def generate_rsa_openssh_keypair(key_size=2048, passphrase=None):
    private_key = rsa.generate_private_key(
        public_exponent=65537,
        key_size=key_size
    )

    if passphrase is None:
        encryption_algorithm = serialization.NoEncryption()
    else:
        if isinstance(passphrase, str):
            passphrase = passphrase.encode('utf-8')
        encryption_algorithm = serialization.BestAvailableEncryption(passphrase)

    private_pem = private_key.private_bytes(
        encoding=serialization.Encoding.PEM,
        format=serialization.PrivateFormat.OpenSSH,
        encryption_algorithm=encryption_algorithm
    )

    public_key = private_key.public_key()
    public_openssh = public_key.public_bytes(
        encoding=serialization.Encoding.OpenSSH,
        format=serialization.PublicFormat.OpenSSH
    )

    return {
        'private_key': private_pem,
        'public_key': public_openssh
    }