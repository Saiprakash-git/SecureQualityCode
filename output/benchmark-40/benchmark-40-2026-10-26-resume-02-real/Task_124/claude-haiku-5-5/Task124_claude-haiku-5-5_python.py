import getpass
import hashlib
import hmac
import json
import os
import secrets
import sys
import uuid

PBKDF2_ITERATIONS = 200000
SALT_LEN = 16
NONCE_LEN = 16
TAG_LEN = 32
DEFAULT_STORE_PATH = "secure_vault.dat"


def derive_keys(password, salt):
    material = hashlib.pbkdf2_hmac(
        "sha256", password.encode("utf-8"), salt, PBKDF2_ITERATIONS, dklen=64
    )
    return material[:32], material[32:]


def _keystream(key, nonce, length):
    out = bytearray()
    counter = 0
    while len(out) < length:
        block = hmac.new(key, nonce + counter.to_bytes(8, "big"), hashlib.sha256).digest()
        out.extend(block)
        counter += 1
    return bytes(out[:length])


def encrypt_bytes(plaintext, password):
    salt = secrets.token_bytes(SALT_LEN)
    nonce = secrets.token_bytes(NONCE_LEN)
    enc_key, mac_key = derive_keys(password, salt)
    ks = _keystream(enc_key, nonce, len(plaintext))
    ciphertext = bytes(a ^ b for a, b in zip(plaintext, ks))
    tag = hmac.new(mac_key, salt + nonce + ciphertext, hashlib.sha256).digest()
    return salt + nonce + ciphertext + tag


def decrypt_bytes(blob, password):
    if len(blob) < SALT_LEN + NONCE_LEN + TAG_LEN:
        raise ValueError("Stored data is too short or corrupted.")
    salt = blob[:SALT_LEN]
    nonce = blob[SALT_LEN:SALT_LEN + NONCE_LEN]
    ciphertext = blob[SALT_LEN + NONCE_LEN:-TAG_LEN]
    tag = blob[-TAG_LEN:]
    enc_key, mac_key = derive_keys(password, salt)
    expected = hmac.new(mac_key, salt + nonce + ciphertext, hashlib.sha256).digest()
    if not hmac.compare_digest(tag, expected):
        raise ValueError("Wrong password or corrupted data.")
    ks = _keystream(enc_key, nonce, len(ciphertext))
    return bytes(a ^ b for a, b in zip(ciphertext, ks))


def load_store(path, password):
    if not os.path.exists(path):
        return {}
    with open(path, "rb") as f:
        blob = f.read()
    plaintext = decrypt_bytes(blob, password)
    return json.loads(plaintext.decode("utf-8"))


def save_store(path, password, store):
    blob = encrypt_bytes(json.dumps(store).encode("utf-8"), password)
    tmp_path = path + ".tmp"
    with open(tmp_path, "wb") as f:
        f.write(blob)
    try:
        os.chmod(tmp_path, 0o600)
    except OSError:
        pass
    os.replace(tmp_path, path)


def luhn_valid(number):
    digits = [int(d) for d in number]
    if len(digits) < 12:
        return False
    total = 0
    parity = len(digits) % 2
    for i, d in enumerate(digits):
        if i % 2 == parity:
            d *= 2
            if d > 9:
                d -= 9
        total += d
    return total % 10 == 0


def mask_card(number):
    return "**** **** **** " + number[-4:]


def mask_value(value):
    if len(value) <= 4:
        return "*" * len(value)
    return "*" * (len(value) - 4) + value[-4:]


def add_record(store, name, fields):
    if not name:
        raise ValueError("Name is required.")
    clean = {}
    for key, value in fields.items():
        if value is None or str(value).strip() == "":
            continue
        value = str(value).strip()
        if key == "card_number":
            value = value.replace(" ", "").replace("-", "")
            if not value.isdigit() or not luhn_valid(value):
                raise ValueError("Invalid credit card number.")
        clean[key] = value
    record_id = uuid.uuid4().hex[:12]
    store[record_id] = {"name": name.strip(), "fields": clean}
    return record_id


def get_record(store, record_id):
    if record_id not in store:
        raise KeyError("Record not found.")
    return json.loads(json.dumps(store[record_id]))


def delete_record(store, record_id):
    if record_id in store:
        del store[record_id]
        return True
    return False


def list_records(store):
    summaries = []
    for record_id, rec in store.items():
        masked = {}
        for key, value in rec["fields"].items():
            if key == "card_number":
                masked[key] = mask_card(value)
            else:
                masked[key] = mask_value(value)
        summaries.append({"id": record_id, "name": rec["name"], "fields": masked})
    return summaries


def _prompt(label, required=False):
    while True:
        value = input(label + ": ").strip()
        if value or not required:
            return value
        print("This field is required.")


def run_cli(path=DEFAULT_STORE_PATH):
    password = getpass.getpass("Vault password: ")
    try:
        store = load_store(path, password)
    except ValueError as e:
        print("Error:", e)
        return 1

    while True:
        print("\n1) Add record  2) List records  3) View record  4) Delete record  5) Quit")
        choice = input("Choose: ").strip()
        try:
            if choice == "1":
                name = _prompt("Full name", required=True)
                fields = {
                    "card_number": _prompt("Credit card number"),
                    "ssn": _prompt("SSN / national ID"),
                    "email": _prompt("Email"),
                    "phone": _prompt("Phone"),
                    "address": _prompt("Address"),
                }
                rid = add_record(store, name, fields)
                save_store(path, password, store)
                print("Saved record", rid)
            elif choice == "2":
                for item in list_records(store):
                    print(item["id"], item["name"], item["fields"])
            elif choice == "3":
                rid = _prompt("Record ID", required=True)
                rec = get_record(store, rid)
                print(json.dumps(rec, indent=2))
            elif choice == "4":
                rid = _prompt("Record ID", required=True)
                if delete_record(store, rid):
                    save_store(path, password, store)
                    print("Deleted.")
                else:
                    print("Record not found.")
            elif choice == "5":
                break
            else:
                print("Invalid choice.")
        except (ValueError, KeyError) as e:
            print("Error:", e)
    return 0


if __name__ == "__main__":
    sys.exit(run_cli(sys.argv[1] if len(sys.argv) > 1 else DEFAULT_STORE_PATH))