import os
import json
import sqlite3
import secrets
from datetime import datetime, timezone
from typing import Optional, Dict, Any, List
from cryptography.hazmat.primitives.ciphers.aead import AESGCM
from cryptography.hazmat.primitives.kdf.pbkdf2 import PBKDF2HMAC
from cryptography.hazmat.primitives import hashes


def derive_key(passphrase: str, salt: bytes, iterations: int = 600000) -> bytes:
    kdf = PBKDF2HMAC(
        algorithm=hashes.SHA256(),
        length=32,
        salt=salt,
        iterations=iterations,
    )
    return kdf.derive(passphrase.encode("utf-8"))


def encrypt_payload(data: dict, key: bytes, associated_data: Optional[bytes] = None) -> bytes:
    serialized = json.dumps(data).encode("utf-8")
    aesgcm = AESGCM(key)
    nonce = secrets.token_bytes(12)
    ciphertext = aesgcm.encrypt(nonce, serialized, associated_data)
    return nonce + ciphertext


def decrypt_payload(encrypted_blob: bytes, key: bytes, associated_data: Optional[bytes] = None) -> dict:
    if len(encrypted_blob) < 12:
        raise ValueError("Invalid encrypted payload: blob too short.")
    nonce = encrypted_blob[:12]
    ciphertext = encrypted_blob[12:]
    aesgcm = AESGCM(key)
    decrypted_bytes = aesgcm.decrypt(nonce, ciphertext, associated_data)
    return json.loads(decrypted_bytes.decode("utf-8"))


def mask_credit_card(card_number: str) -> str:
    cleaned = "".join(filter(str.isdigit, card_number))
    if len(cleaned) < 4:
        return "****"
    return f"{'*' * (len(cleaned) - 4)}{cleaned[-4:]}"


def init_vault(db_path: str, passphrase: str) -> tuple[sqlite3.Connection, bytes]:
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()

    cursor.execute("""
        CREATE TABLE IF NOT EXISTS vault_metadata (
            id INTEGER PRIMARY KEY CHECK (id = 1),
            salt BLOB NOT NULL,
            created_at TEXT NOT NULL
        )
    """)

    cursor.execute("""
        CREATE TABLE IF NOT EXISTS secrets (
            identifier TEXT PRIMARY KEY,
            category TEXT NOT NULL,
            masked_hint TEXT NOT NULL,
            encrypted_data BLOB NOT NULL,
            created_at TEXT NOT NULL,
            updated_at TEXT NOT NULL
        )
    """)
    conn.commit()

    cursor.execute("SELECT salt FROM vault_metadata WHERE id = 1")
    row = cursor.fetchone()

    if row is None:
        salt = secrets.token_bytes(16)
        now_utc = datetime.now(timezone.utc).isoformat()
        cursor.execute(
            "INSERT INTO vault_metadata (id, salt, created_at) VALUES (1, ?, ?)",
            (salt, now_utc)
        )
        conn.commit()
    else:
        salt = row[0]

    key = derive_key(passphrase, salt)
    return conn, key


def store_secret(
    conn: sqlite3.Connection,
    key: bytes,
    identifier: str,
    category: str,
    payload: Dict[str, Any],
    masked_hint: str
) -> bool:
    encrypted_blob = encrypt_payload(payload, key, associated_data=identifier.encode("utf-8"))
    now_utc = datetime.now(timezone.utc).isoformat()

    cursor = conn.cursor()
    cursor.execute("""
        INSERT INTO secrets (identifier, category, masked_hint, encrypted_data, created_at, updated_at)
        VALUES (?, ?, ?, ?, ?, ?)
        ON CONFLICT(identifier) DO UPDATE SET
            category = excluded.category,
            masked_hint = excluded.masked_hint,
            encrypted_data = excluded.encrypted_data,
            updated_at = excluded.updated_at
    """, (identifier, category, masked_hint, encrypted_blob, now_utc, now_utc))
    conn.commit()
    return True


def retrieve_secret(
    conn: sqlite3.Connection,
    key: bytes,
    identifier: str
) -> Optional[Dict[str, Any]]:
    cursor = conn.cursor()
    cursor.execute("SELECT encrypted_data FROM secrets WHERE identifier = ?", (identifier,))
    row = cursor.fetchone()
    if row is None:
        return None

    encrypted_blob = row[0]
    return decrypt_payload(encrypted_blob, key, associated_data=identifier.encode("utf-8"))


def delete_secret(conn: sqlite3.Connection, identifier: str) -> bool:
    cursor = conn.cursor()
    cursor.execute("DELETE FROM secrets WHERE identifier = ?", (identifier,))
    conn.commit()
    return cursor.rowcount > 0


def list_secrets(conn: sqlite3.Connection) -> List[Dict[str, str]]:
    cursor = conn.cursor()
    cursor.execute("SELECT identifier, category, masked_hint, created_at, updated_at FROM secrets ORDER BY created_at ASC")
    rows = cursor.fetchall()
    return [
        {
            "identifier": r[0],
            "category": r[1],
            "masked_hint": r[2],
            "created_at": r[3],
            "updated_at": r[4]
        }
        for r in rows
    ]


def close_vault(conn: sqlite3.Connection) -> None:
    conn.close()


def run_demonstration() -> None:
    db_filename = "secure_vault_demo.db"
    master_passphrase = "CorrectHorseBatteryStaple#2026!"

    if os.path.exists(db_filename):
        os.remove(db_filename)

    conn, vault_key = init_vault(db_filename, master_passphrase)

    cc_identifier = "card_alice_primary"
    raw_card_number = "4111222233334444"
    card_data = {
        "holder_name": "Alice Smith",
        "card_number": raw_card_number,
        "expiry": "12/28",
        "cvv": "987"
    }
    store_secret(
        conn=conn,
        key=vault_key,
        identifier=cc_identifier,
        category="credit_card",
        payload=card_data,
        masked_hint=mask_credit_card(raw_card_number)
    )

    ssn_identifier = "pii_bob_tax"
    bob_data = {
        "full_name": "Bob Jones",
        "tax_id": "987-65-4321",
        "date_of_birth": "1985-04-12"
    }
    store_secret(
        conn=conn,
        key=vault_key,
        identifier=ssn_identifier,
        category="pii",
        payload=bob_data,
        masked_hint="SSN ending in 4321"
    )

    metadata_list = list_secrets(conn)
    print("Stored Secret Metadata:")
    for entry in metadata_list:
        print(f" - ID: {entry['identifier']} | Type: {entry['category']} | Info: {entry['masked_hint']}")

    decrypted_card = retrieve_secret(conn, vault_key, cc_identifier)
    print(f"\nDecrypted Credit Card Data for '{cc_identifier}':")
    print(json.dumps(decrypted_card, indent=2))

    decrypted_ssn = retrieve_secret(conn, vault_key, ssn_identifier)
    print(f"\nDecrypted PII Data for '{ssn_identifier}':")
    print(json.dumps(decrypted_ssn, indent=2))

    close_vault(conn)

    reopened_conn, reopened_key = init_vault(db_filename, master_passphrase)
    reloaded_card = retrieve_secret(reopened_conn, reopened_key, cc_identifier)
    assert reloaded_card == card_data, "Verification check failed: data mismatch after reopen."

    delete_secret(reopened_conn, cc_identifier)
    remaining_records = list_secrets(reopened_conn)
    print(f"\nRemaining secret count after deletion: {len(remaining_records)}")

    close_vault(reopened_conn)
    if os.path.exists(db_filename):
        os.remove(db_filename)


if __name__ == "__main__":
    run_demonstration()