import base64
import math
import secrets
import struct


def _is_probable_prime(n, rounds=40):
    if n < 2:
        return False
    small_primes = (2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37)
    for p in small_primes:
        if n % p == 0:
            return n == p
    d = n - 1
    s = 0
    while d % 2 == 0:
        d //= 2
        s += 1
    for _ in range(rounds):
        a = secrets.randbelow(n - 3) + 2
        x = pow(a, d, n)
        if x == 1 or x == n - 1:
            continue
        for _ in range(s - 1):
            x = pow(x, 2, n)
            if x == n - 1:
                break
        else:
            return False
    return True


def _generate_prime(bits):
    while True:
        candidate = secrets.randbits(bits) | (1 << (bits - 1)) | (1 << (bits - 2)) | 1
        if _is_probable_prime(candidate):
            return candidate


def generate_rsa_components(bits=2048, public_exponent=65537):
    if bits < 1024 or bits % 2 != 0:
        raise ValueError("bits must be an even number >= 1024")
    e = public_exponent
    while True:
        p = _generate_prime(bits // 2)
        q = _generate_prime(bits - bits // 2)
        if p == q:
            continue
        if math.gcd(e, p - 1) != 1 or math.gcd(e, q - 1) != 1:
            continue
        n = p * q
        if n.bit_length() != bits:
            continue
        break
    phi = (p - 1) * (q - 1)
    d = pow(e, -1, phi)
    iqmp = pow(q, -1, p)
    return {"n": n, "e": e, "d": d, "p": p, "q": q, "iqmp": iqmp}


def _string(data):
    return struct.pack(">I", len(data)) + data


def _mpint(value):
    if value == 0:
        raw = b""
    else:
        raw = value.to_bytes((value.bit_length() + 7) // 8, "big")
        if raw[0] & 0x80:
            raw = b"\x00" + raw
    return struct.pack(">I", len(raw)) + raw


def export_openssh_rsa_private_key(components, comment=""):
    n = components["n"]
    e = components["e"]
    d = components["d"]
    p = components["p"]
    q = components["q"]
    iqmp = components["iqmp"]

    public_blob = _string(b"ssh-rsa") + _mpint(e) + _mpint(n)

    check = secrets.randbits(32)
    private_blob = (
        struct.pack(">II", check, check)
        + _string(b"ssh-rsa")
        + _mpint(n)
        + _mpint(e)
        + _mpint(d)
        + _mpint(iqmp)
        + _mpint(p)
        + _mpint(q)
        + _string(comment.encode("utf-8"))
    )
    pad_len = (-len(private_blob)) % 8
    private_blob += bytes(range(1, pad_len + 1))

    body = (
        b"openssh-key-v1\x00"
        + _string(b"none")
        + _string(b"none")
        + _string(b"")
        + struct.pack(">I", 1)
        + _string(public_blob)
        + _string(private_blob)
    )

    b64 = base64.b64encode(body).decode("ascii")
    lines = [b64[i:i + 70] for i in range(0, len(b64), 70)]
    return (
        "-----BEGIN OPENSSH PRIVATE KEY-----\n"
        + "\n".join(lines)
        + "\n-----END OPENSSH PRIVATE KEY-----\n"
    )


def export_openssh_rsa_public_key(components, comment=""):
    n = components["n"]
    e = components["e"]
    public_blob = _string(b"ssh-rsa") + _mpint(e) + _mpint(n)
    line = "ssh-rsa " + base64.b64encode(public_blob).decode("ascii")
    if comment:
        line += " " + comment
    return line + "\n"


def generate_openssh_rsa_key(bits=2048, comment=""):
    components = generate_rsa_components(bits)
    private_key = export_openssh_rsa_private_key(components, comment)
    public_key = export_openssh_rsa_public_key(components, comment)
    return {"private_key": private_key, "public_key": public_key}


if __name__ == "__main__":
    result = generate_openssh_rsa_key(2048, "user@host")
    print(result["private_key"], end="")