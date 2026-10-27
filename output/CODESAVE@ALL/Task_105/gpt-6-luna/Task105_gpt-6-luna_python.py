from __future__ import annotations

import argparse
import json
import socket
import ssl
import sys
from typing import Optional, Sequence


def create_tls_context(
    cafile: Optional[str] = None,
    capath: Optional[str] = None,
    cadata: Optional[str] = None,
    certfile: Optional[str] = None,
    keyfile: Optional[str] = None,
    alpn_protocols: Optional[Sequence[str]] = None,
) -> ssl.SSLContext:
    """Create a TLS client context that requires trusted certificates and valid hostnames."""
    context = ssl.create_default_context(
        purpose=ssl.Purpose.SERVER_AUTH,
        cafile=cafile,
        capath=capath,
        cadata=cadata,
    )
    context.minimum_version = ssl.TLSVersion.TLSv1_2

    if hasattr(context, "hostname_checks_common_name"):
        context.hostname_checks_common_name = False

    if certfile is not None:
        context.load_cert_chain(certfile=certfile, keyfile=keyfile)
    elif keyfile is not None:
        raise ValueError("keyfile requires certfile")

    if alpn_protocols is not None:
        protocols = list(alpn_protocols)
        if any(not isinstance(protocol, str) or not protocol for protocol in protocols):
            raise ValueError("ALPN protocols must be non-empty strings")
        context.set_alpn_protocols(protocols)

    return context


def connect_tls(
    host: str,
    port: int = 443,
    timeout: Optional[float] = 10.0,
    cafile: Optional[str] = None,
    capath: Optional[str] = None,
    cadata: Optional[str] = None,
    certfile: Optional[str] = None,
    keyfile: Optional[str] = None,
    server_hostname: Optional[str] = None,
    alpn_protocols: Optional[Sequence[str]] = None,
) -> ssl.SSLSocket:
    """Open a TLS connection with certificate and hostname verification enabled."""
    if not isinstance(host, str) or not host:
        raise ValueError("host must be a non-empty string")
    if isinstance(port, bool) or not isinstance(port, int) or not 1 <= port <= 65535:
        raise ValueError("port must be an integer from 1 to 65535")

    identity = server_hostname if server_hostname is not None else host
    if not isinstance(identity, str) or not identity:
        raise ValueError("server_hostname must be a non-empty string")

    context = create_tls_context(
        cafile=cafile,
        capath=capath,
        cadata=cadata,
        certfile=certfile,
        keyfile=keyfile,
        alpn_protocols=alpn_protocols,
    )

    raw_socket = socket.create_connection((host, port), timeout=timeout)
    try:
        return context.wrap_socket(raw_socket, server_hostname=identity)
    except BaseException:
        raw_socket.close()
        raise


def connection_info(connection: ssl.SSLSocket) -> dict:
    """Return basic information about an established TLS connection."""
    return {
        "peer": connection.getpeername(),
        "tls_version": connection.version(),
        "cipher": connection.cipher(),
        "alpn_protocol": connection.selected_alpn_protocol(),
        "peer_certificate": connection.getpeercert(),
    }


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description="Establish and verify a TLS connection.")
    parser.add_argument("host", help="Remote server hostname or IP address")
    parser.add_argument("--port", type=int, default=443, help="Remote server port (default: 443)")
    parser.add_argument("--timeout", type=float, default=10.0, help="Connection timeout in seconds")
    parser.add_argument("--cafile", help="Additional or custom CA certificate file")
    parser.add_argument("--capath", help="Directory containing CA certificates")
    parser.add_argument("--certfile", help="Client certificate file for mutual TLS")
    parser.add_argument("--keyfile", help="Client private key file")
    parser.add_argument(
        "--alpn",
        nargs="+",
        help="ALPN protocols to offer, for example: h2 http/1.1",
    )
    args = parser.parse_args(argv)

    try:
        with connect_tls(
            host=args.host,
            port=args.port,
            timeout=args.timeout,
            cafile=args.cafile,
            capath=args.capath,
            certfile=args.certfile,
            keyfile=args.keyfile,
            alpn_protocols=args.alpn,
        ) as connection:
            print(json.dumps(connection_info(connection), indent=2, default=str))
        return 0
    except (OSError, ssl.SSLError, ValueError) as error:
        print("TLS connection failed: {}".format(error), file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())