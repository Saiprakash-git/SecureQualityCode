import socket
import ssl
import sys
import argparse


def create_secure_context(cafile=None, verify=True, min_version=ssl.TLSVersion.TLSv1_2):
    context = ssl.create_default_context(purpose=ssl.Purpose.SERVER_AUTH, cafile=cafile)
    context.minimum_version = min_version
    if not verify:
        context.check_hostname = False
        context.verify_mode = ssl.CERT_NONE
    else:
        context.check_hostname = True
        context.verify_mode = ssl.CERT_REQUIRED
    context.options |= ssl.OP_NO_COMPRESSION
    return context


def establish_tls_connection(host, port=443, timeout=10.0, cafile=None, verify=True,
                             server_name=None, min_version=ssl.TLSVersion.TLSv1_2):
    context = create_secure_context(cafile=cafile, verify=verify, min_version=min_version)
    sni = server_name or host
    raw_sock = socket.create_connection((host, port), timeout=timeout)
    try:
        tls_sock = context.wrap_socket(raw_sock, server_hostname=sni)
    except Exception:
        raw_sock.close()
        raise
    return tls_sock


def get_connection_info(tls_sock):
    cert = tls_sock.getpeercert()
    info = {
        "protocol": tls_sock.version(),
        "cipher": tls_sock.cipher(),
        "server_hostname": tls_sock.server_hostname,
        "certificate": cert,
    }
    return info


def send_request(tls_sock, host, path="/"):
    request = (
        f"GET {path} HTTP/1.1\r\n"
        f"Host: {host}\r\n"
        "User-Agent: secure-tls-client/1.0\r\n"
        "Connection: close\r\n\r\n"
    ).encode("ascii")
    tls_sock.sendall(request)
    chunks = []
    while True:
        data = tls_sock.recv(4096)
        if not data:
            break
        chunks.append(data)
    return b"".join(chunks)


def fetch_secure(host, port=443, path="/", timeout=10.0, cafile=None, verify=True):
    with establish_tls_connection(host, port, timeout, cafile, verify) as tls_sock:
        info = get_connection_info(tls_sock)
        response = send_request(tls_sock, host, path)
    return info, response


def parse_args(argv):
    parser = argparse.ArgumentParser(description="Establish a secure SSL/TLS connection to a remote server.")
    parser.add_argument("host", help="Server hostname")
    parser.add_argument("-p", "--port", type=int, default=443, help="Server port (default 443)")
    parser.add_argument("-P", "--path", default="/", help="HTTP path to request (default /)")
    parser.add_argument("-t", "--timeout", type=float, default=10.0, help="Timeout in seconds")
    parser.add_argument("-c", "--cafile", default=None, help="Custom CA bundle file")
    parser.add_argument("--no-verify", action="store_true", help="Disable certificate verification (insecure)")
    parser.add_argument("--body", action="store_true", help="Print full response body")
    return parser.parse_args(argv)


def main(argv=None):
    args = parse_args(argv if argv is not None else sys.argv[1:])
    try:
        info, response = fetch_secure(
            args.host,
            port=args.port,
            path=args.path,
            timeout=args.timeout,
            cafile=args.cafile,
            verify=not args.no_verify,
        )
    except ssl.SSLCertVerificationError as e:
        print(f"Certificate verification failed: {e}", file=sys.stderr)
        return 1
    except ssl.SSLError as e:
        print(f"TLS error: {e}", file=sys.stderr)
        return 1
    except (socket.error, OSError) as e:
        print(f"Connection error: {e}", file=sys.stderr)
        return 1

    print(f"Protocol: {info['protocol']}")
    print(f"Cipher: {info['cipher']}")
    subject = info["certificate"].get("subject",()) if info["certificate"] else ()
    print(f"Certificate subject: {subject}")
    issuer = info["certificate"].get("issuer", ()) if info["certificate"] else ()
    print(f"Certificate issuer: {issuer}")
    not_after = info["certificate"].get("notAfter") if info["certificate"] else None
    print(f"Certificate expires: {not_after}")

    text = response.decode("utf-8", errors="replace")
    header_end = text.find("\r\n\r\n")
    if header_end != -1:
        status_line = text.split("\r\n", 1)[0]
        print(f"Response status: {status_line}")
        if args.body:
            print(text[header_end + 4:])
    else:
        print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())