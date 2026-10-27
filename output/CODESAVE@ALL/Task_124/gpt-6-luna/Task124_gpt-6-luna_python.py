import base64
import json
import os
import secrets
import sqlite3
import stat
import uuid
from pathlib import Path
from typing import Any, Optional

from cryptography.fernet import Fernet, InvalidToken
from cryptography.hazmat.primitives.kdf.scrypt import Scrypt


class SecureVault:
    """Store JSON-compatible records encrypted in a local SQLite database."""

    _SCRYPT_N = 1 << 15
    _SCRYPT_R = 8
    _SCRYPT_P = 1
    _SALT_LENGTH = 16

    def __init__(self, database_path: str, passphrase: str) -> None:
        if not isinstance(passphrase, str) or not passphrase:
            raise ValueError("A non-empty passphrase is required.")

        self._path = Path(database_path).expanduser()
        self._path.parent.mkdir(parents=True, exist_ok=True)

        if self._path.is_symlink():
            raise ValueError("The database path must not be a symbolic link.")

        if not self._path.exists():
            try:
                fd = os.open(
                    str(self._path),
                    os.O_CREAT | os.O_EXCL | os.O_RDWR,
                    stat.S_IRUSR | stat.S_IWUSR,
                )
                os.close(fd)
            except FileExistsError:
                pass

        os.chmod(self._path, stat.S_IRUSR | stat.S_IWUSR)
        self._connection = sqlite3.connect(str(self._path), timeout=30)
        self._connection.execute("PRAGMA secure_delete = ON")
        self._connection.execute("PRAGMA journal_mode = DELETE")
        self._connection.execute(
            "CREATE TABLE IF NOT EXISTS vault_meta "
            "(name TEXT PRIMARY KEY, value BLOB NOT NULL)"
        )
        self._connection.execute(
            "CREATE TABLE IF NOT EXISTS vault_records "
            "(record_id TEXT PRIMARY KEY, ciphertext BLOB NOT NULL)"
        )

        row = self._connection.execute(
            "SELECT value FROM vault_meta WHERE name = ?",
            ("salt",),
        ).fetchone()

        if row is None:
            existing_record = self._connection.execute(
                "SELECT 1 FROM vault_records LIMIT 1"
            ).fetchone()
            if existing_record is not None:
                self._connection.close()
                raise ValueError("Vault metadata is missing; refusing to initialize.")

            salt = secrets.token_bytes(self._SALT_LENGTH)
            with self._connection:
                self._connection.execute(
                    "INSERT INTO vault_meta (name, value) VALUES (?, ?)",
                    ("salt", salt),
                )
        else:
            salt = bytes(row[0])

        key = Scrypt(
            salt=salt,
            length=32,
            n=self._SCRYPT_N,
            r=self._SCRYPT_R,
            p=self._SCRYPT_P,
        ).derive(passphrase.encode("utf-8"))
        self._cipher = Fernet(base64.urlsafe_b64encode(key))
        self._closed = False

    def store(self, data: Any) -> str:
        """Encrypt and store a JSON-compatible value; return its opaque ID."""
        self._ensure_open()
        serialized = json.dumps(
            data,
            ensure_ascii=False,
            separators=(",", ":"),
            allow_nan=False,
        ).encode("utf-8")
        ciphertext = self._cipher.encrypt(serialized)
        record_id = uuid.uuid4().hex

        with self._connection:
            self._connection.execute(
                "INSERT INTO vault_records (record_id, ciphertext) VALUES (?, ?)",
                (record_id, ciphertext),
            )
        return record_id

    def retrieve(self, record_id: str) -> Optional[Any]:
        """Return a decrypted value, or None if the ID does not exist."""
        self._ensure_open()
        row = self._connection.execute(
            "SELECT ciphertext FROM vault_records WHERE record_id = ?",
            (record_id,),
        ).fetchone()
        if row is None:
            return None

        try:
            plaintext = self._cipher.decrypt(bytes(row[0]))
            return json.loads(plaintext.decode("utf-8"))
        except (InvalidToken, UnicodeDecodeError, json.JSONDecodeError) as exc:
            raise ValueError("Record cannot be decrypted.") from exc

    def delete(self, record_id: str) -> bool:
        """Delete a record and report whether it existed."""
        self._ensure_open()
        with self._connection:
            cursor = self._connection.execute(
                "DELETE FROM vault_records WHERE record_id = ?",
                (record_id,),
            )
        return cursor.rowcount > 0

    def list_ids(self) -> list[str]:
        """Return record IDs without decrypting record contents."""
        self._ensure_open()
        rows = self._connection.execute(
            "SELECT record_id FROM vault_records ORDER BY record_id"
        ).fetchall()
        return [row[0] for row in rows]

    def close(self) -> None:
        if not self._closed:
            self._connection.close()
            self._closed = True

    def _ensure_open(self) -> None:
        if self._closed:
            raise RuntimeError("Vault is closed.")

    def __enter__(self) -> "SecureVault":
        self._ensure_open()
        return self

    def __exit__(self, exc_type: Any, exc_value: Any, traceback: Any) -> None:
        self.close()