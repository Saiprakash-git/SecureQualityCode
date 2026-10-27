import os
import json
import base64
from cryptography.fernet import Fernet
from cryptography.hazmat.primitives import hashes
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2
from cryptography.hazmat.backends import default_backend

class SecureDataStore:
    def __init__(self, storage_file, master_password):
        self.storage_file = storage_file
        self.salt_file = storage_file + ".salt"
        self.salt = self._get_or_create_salt()
        self.key = self._derive_key(master_password, self.salt)
        self.cipher = Fernet(self.key)

    def _get_or_create_salt(self):
        if os.path.exists(self.salt_file):
            with open(self.salt_file, "rb") as f:
                return f.read()
        salt = os.urandom(16)
        with open(self.salt_file, "wb") as f:
            f.write(salt)
        os.chmod(self.salt_file, 0o600)
        return salt

    def _derive_key(self, password, salt):
        kdf = PBKDF2(
            algorithm=hashes.SHA256(),
            length=32,
            salt=salt,
            iterations=480000,
            backend=default_backend()
        )
        return base64.urlsafe_b64encode(kdf.derive(password.encode()))

    def _load_store(self):
        if not os.path.exists(self.storage_file):
            return {}
        with open(self.storage_file, "r") as f:
            return json.load(f)

    def _save_store(self, data):
        with open(self.storage_file, "w") as f:
            json.dump(data, f)
        os.chmod(self.storage_file, 0o600)

    def store(self, key, value):
        encrypted = self.cipher.encrypt(value.encode())
        data = self._load_store()
        data[key] = base64.b64encode(encrypted).decode()
        self._save_store(data)

    def retrieve(self, key):
        data = self._load_store()
        if key not in data:
            raise KeyError(f"Key {key} not found")
        encrypted = base64.b64decode(data[key])
        return self.cipher.decrypt(encrypted).decode()

    def delete(self, key):
        data = self._load_store()
        if key in data:
            del data[key]
            self._save_store(data)

    def clear(self):
        if os.path.exists(self.storage_file):
            os.remove(self.storage_file)

def main():
    store = SecureDataStore("vault.dat", "secure_master_password")
    store.store("credit_card_1", "4111-1111-1111-1111")
    store.store("ssn_1", "123-45-6789")
    print(store.retrieve("credit_card_1"))
    store.delete("ssn_1")

if __name__ == "__main__":
    main()