import socket
import ssl
import sys
from typing import Any, Dict, Optional, Union

DEFAULT_PORT = 443
DEFAULT_TIMEOUT = 10.0
DEFAULT_MAX_RESPONSE_BYTES = 1_000_000


def create_ssl_context(
    cafile: Optional[str] = None,
    capath: Optional[str] = None,
    cadata: Optional[Union[str, bytes]] = None,
    client_cert: Optional[str] = None,
    client_key: Optional[str] = None,
    password: Optional[Union[str, bytes]] = None,
) -> ssl.SSLContext:
    context = ssl.SSLContext(ssl.PROTOCOL_TLS_CLIENT)
    context.check_hostname = True
    context.verify_mode = ssl.CERT_REQUIRED

    try:
        context.minimum_version = ssl.TLSVersion.TLSv1_2
    except AttributeError:
        pass

    if cafile or capath or cadata:
        context.load_verify_locations(
            cafile=cafile,
            capath=capath,
            cadata=cadata,
        )
    else:
        context.load_default_certs()

    if client_cert:
        context.load_cert_chain(
            certfile=client_cert,
            keyfile=client_key,
            password=password,
        )

    return context


def establish_secure_connection(
    host: str,
    port: int = DEFAULT_PORT,
    timeout: Optional[float] = DEFAULT_TIMEOUT,
    cafile: Optional[str] = None,
    capath: Optional[str] = None,
    cadata: Optional[Union[str, bytes]] = None,
    client_cert: Optional[str] = None,
    client_key: Optional[str] = None,
    password: Optional[Union[str, bytes]] = None,
    server_hostname: Optional[str] = None,
) -> ssl.SSLSocket:
    if not isinstance(host, str) or not host.strip():
        raise ValueError("host must be a non-empty string")

    port = int(port)
    if port < 1 or port > 65535:
        raise ValueError("port must be between 1 and 65535")

    context = create_ssl_context(
        cafile=cafile,
        capath=capath,
        cadata=cadata,
        client_cert=client_cert,
        client_key=client_key,
        password=password,
    )

    if server_hostname is None:
        server_hostname = host

    raw_socket = socket.create_connection((host, port), timeout=timeout)

    try:
        secure_socket = context.wrap_socket(raw_socket, server_hostname=server_hostname)
    except Exception:
        raw_socket.close()
        raise

    return secure_socket


def get_connection_info(secure_socket: ssl.SSLSocket) -> Dict[str, Any]:
    peer_certificate = secure_socket.getpeercert()
    peer_certificate_binary = secure_socket.getpeercert(binary_form=True)

    try:
        peer_address = secure_socket.getpeername()
    except OSError:
        peer_address = None

    return {
        "server_hostname": secure_socket.server_hostname,
        "peer_address": peer_address,
        "protocol": secure_socket.version(),
        "cipher": secure_socket.cipher(),
        "peer_certificate": peer_certificate,
        "peer_certificate_binary": peer_certificate_binary,
    }


def secure_request(
    host: str,
    port: int = DEFAULT_PORT,
    request: Union[str, bytes] = "GET / HTTP/1.0\r\n\r\n",
    timeout: Optional[float] = DEFAULT_TIMEOUT,
    max_response_bytes: int = DEFAULT_MAX_RESPONSE_BYTES,
    cafile: Optional[str] = None,
    capath: Optional[str] = None,
    cadata: Optional[Union[str, bytes]] = None,
    client_cert: Optional[str] = None,
    client_key: Optional[str] = None,
    password: Optional[Union[str, bytes]] = None,
    server_hostname: Optional[str] = None,
) -> bytes:
    request_bytes = request.encode("utf-8") if isinstance(request, str) else bytes(request)

    secure_socket = establish_secure_connection(
        host=host,
        port=port,
        timeout=timeout,
        cafile=cafile,
        capath=capath,
        cadata=cadata,
        client_cert=client_cert,
        client_key=client_key,
        password=password,
        server_hostname=server_hostname,
    )

    try:
        secure_socket.sendall(request_bytes)
        secure_socket.settimeout(timeout)

        chunks = []
        total_size = 0

        while True:
            chunk = secure_socket.recv(4096)
            if not chunk:
                break

            total_size += len(chunk)
            if total_size > max_response_bytes:
                raise ValueError("response exceeded maximum allowed size")

            chunks.append(chunk)

        return b"".join(chunks)
    finally:
        secure_socket.close()


def main(argv: Optional[list] = None) -> int:
    if argv is None:
        argv = sys.argv

    if len(argv) < 2:
        print(
            "usage: ssl_client.py <host> [port] [request]",
            file=sys.stderr,
        )
        return 1

    host = argv[1]
    port = int(argv[2]) if len(argv) > 2 else DEFAULT_PORT
    request = argv[3] if len(argv) > 3 else "GET / HTTP/1.0\r\n\r\n"

    try:
        response = secure_request(host=host, port=port, request=request)
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    sys.stdout.buffer.write(response)
    return 0


if __name__ == "__main__":
    sys.exit(main())