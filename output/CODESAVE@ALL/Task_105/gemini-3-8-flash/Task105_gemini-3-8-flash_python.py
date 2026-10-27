import socket
import ssl
from typing import Any, Dict, Optional, Tuple


def create_tls_context(
    verify: bool = True,
    ca_file: Optional[str] = None,
    client_cert: Optional[str] = None,
    client_key: Optional[str] = None,
) -> ssl.SSLContext:
    if verify:
        context = ssl.create_default_context(
            purpose=ssl.Purpose.SERVER_AUTH,
            cafile=ca_file
        )
        context.check_hostname = True
        context.verify_mode = ssl.CERT_REQUIRED
    else:
        context = ssl.create_default_context()
        context.check_hostname = False
        context.verify_mode = ssl.CERT_NONE

    context.minimum_version = ssl.TLSVersion.TLSv1_2

    if client_cert and client_key:
        context.load_cert_chain(certfile=client_cert, keyfile=client_key)

    return context


def establish_tls_connection(
    host: str,
    port: int = 443,
    timeout: float = 10.0,
    context: Optional[ssl.SSLContext] = None,
) -> ssl.SSLSocket:
    if context is None:
        context = create_tls_context()

    raw_socket = socket.create_connection((host, port), timeout=timeout)
    tls_socket = context.wrap_socket(raw_socket, server_hostname=host)
    return tls_socket


def get_connection_metadata(tls_socket: ssl.SSLSocket) -> Dict[str, Any]:
    peer_cert = tls_socket.getpeercert()
    cipher_info = tls_socket.cipher()
    tls_version = tls_socket.version()

    return {
        "tls_version": tls_version,
        "cipher_name": cipher_info[0] if cipher_info else None,
        "cipher_protocol": cipher_info[1] if cipher_info else None,
        "cipher_bits": cipher_info[2] if cipher_info else None,
        "peer_certificate": peer_cert,
    }


def send_and_receive(
    tls_socket: ssl.SSLSocket,
    data: bytes,
    buffer_size: int = 4096,
) -> bytes:
    tls_socket.sendall(data)
    response = bytearray()
    while True:
        try:
            chunk = tls_socket.recv(buffer_size)
            if not chunk:
                break
            response.extend(chunk)
            if len(chunk) < buffer_size:
                break
        except socket.timeout:
            break
    return bytes(response)


def execute_tls_exchange(
    host: str,
    port: int = 443,
    payload: bytes = b"",
    timeout: float = 10.0,
) -> Tuple[Dict[str, Any], bytes]:
    tls_socket = establish_tls_connection(host=host, port=port, timeout=timeout)
    try:
        metadata = get_connection_metadata(tls_socket)
        response = send_and_receive(tls_socket, payload) if payload else b""
        return metadata, response
    finally:
        tls_socket.close()


if __name__ == "__main__":
    target_host = "www.cloudflare.com"
    target_port = 443
    http_request = f"GET / HTTP/1.1\r\nHost: {target_host}\r\nConnection: close\r\n\r\n".encode("utf-8")

    meta, res = execute_tls_exchange(target_host, target_port, payload=http_request)
    print(f"Connected to: {target_host}:{target_port}")
    print(f"TLS Protocol Version: {meta['tls_version']}")
    print(f"Cipher Suite: {meta['cipher_name']} ({meta['cipher_bits']} bits)")
    print(f"Response Length: {len(res)} bytes")
    print("Response Header Preview:")
    print(res.decode("utf-8", errors="replace")[:300])