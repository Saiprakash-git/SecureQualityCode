import ssl
import socket
from typing import Optional, Tuple, Dict, Any, Union

def create_ssl_context(verify_mode: int = ssl.CERT_REQUIRED,
                       check_hostname: bool = True,
                       protocol: int = ssl.PROTOCOL_TLS_CLIENT,
                       cafile: Optional[str] = None,
                       capath: Optional[str] = None,
                       cadata: Optional[str] = None) -> ssl.SSLContext:
    context = ssl.SSLContext(protocol)
    context.verify_mode = verify_mode
    context.check_hostname = check_hostname
    if cafile or capath or cadata:
        context.load_verify_locations(cafile=cafile, capath=capath, cadata=cadata)
    else:
        context.load_default_certs()
    return context

def connect_to_server(hostname: str,
                      port: int,
                      context: Optional[ssl.SSLContext] = None,
                      timeout: float = 10.0,
                      server_hostname: Optional[str] = None) -> ssl.SSLSocket:
    if context is None:
        context = create_ssl_context()
    sock = socket.create_connection((hostname, port), timeout=timeout)
    ssl_sock = context.wrap_socket(sock, server_hostname=server_hostname or hostname)
    return ssl_sock

def send_data(ssl_socket: ssl.SSLSocket, data: Union[bytes, str], encoding: str = 'utf-8') -> int:
    if isinstance(data, str):
        data = data.encode(encoding)
    return ssl_socket.send(data)

def send_all(ssl_socket: ssl.SSLSocket, data: Union[bytes, str], encoding: str = 'utf-8') -> int:
    if isinstance(data, str):
        data = data.encode(encoding)
    return ssl_socket.sendall(data)

def receive_data(ssl_socket: ssl.SSLSocket, buffer_size: int = 4096, encoding: Optional[str] = None) -> Union[bytes, str]:
    data = ssl_socket.recv(buffer_size)
    if encoding:
        return data.decode(encoding)
    return data

def receive_until(ssl_socket: ssl.SSLSocket, delimiter: bytes = b'\r\n', max_size: int = 65536, encoding: Optional[str] = None) -> Union[bytes, str]:
    buffer = b''
    while delimiter not in buffer and len(buffer) < max_size:
        chunk = ssl_socket.recv(1)
        if not chunk:
            break
        buffer += chunk
    if encoding:
        return buffer.decode(encoding)
    return buffer

def get_peer_certificate(ssl_socket: ssl.SSLSocket, binary_form: bool = False) -> Optional[Union[Dict[str, Any], bytes]]:
    return ssl_socket.getpeercert(binary_form=binary_form)

def get_cipher_info(ssl_socket: ssl.SSLSocket) -> Optional[Tuple[str, str, int]]:
    return ssl_socket.cipher()

def get_tls_version(ssl_socket: ssl.SSLSocket) -> Optional[str]:
    return ssl_socket.version()

def get_connection_info(ssl_socket: ssl.SSLSocket) -> Dict[str, Any]:
    return {
        'peer_certificate': get_peer_certificate(ssl_socket),
        'cipher': get_cipher_info(ssl_socket),
        'tls_version': get_tls_version(ssl_socket),
        'compression': ssl_socket.compression(),
        'server_hostname': ssl_socket.server_hostname
    }

def close_connection(ssl_socket: ssl.SSLSocket) -> None:
    try:
        ssl_socket.unwrap()
    except (ssl.SSLError, OSError):
        pass
    finally:
        ssl_socket.close()

def establish_secure_connection(hostname: str,
                                port: int,
                                timeout: float = 10.0,
                                verify: bool = True,
                                cafile: Optional[str] = None,
                                capath: Optional[str] = None,
                                cadata: Optional[str] = None,
                                server_hostname: Optional[str] = None) -> Tuple[ssl.SSLSocket, Dict[str, Any]]:
    context = create_ssl_context(
        verify_mode=ssl.CERT_REQUIRED if verify else ssl.CERT_NONE,
        check_hostname=verify,
        cafile=cafile,
        capath=capath,
        cadata=cadata
    )
    ssl_sock = connect_to_server(hostname, port, context, timeout, server_hostname)
    info = get_connection_info(ssl_sock)
    return ssl_sock, info

def send_https_request(hostname: str,
                       port: int = 443,
                       path: str = '/',
                       method: str = 'GET',
                       headers: Optional[Dict[str, str]] = None,
                       body: Optional[Union[bytes, str]] = None,
                       timeout: float = 10.0,
                       verify: bool = True) -> Tuple[int, Dict[str, str], bytes]:
    ssl_sock, _ = establish_secure_connection(hostname, port, timeout, verify)
    try:
        request_lines = [f"{method} {path} HTTP/1.1", f"Host: {hostname}"]
        if headers:
            for key, value in headers.items():
                request_lines.append(f"{key}: {value}")
        request_lines.append("Connection: close")
        request_lines.append("")
        request = "\r\n".join(request_lines)
        if body:
            if isinstance(body, str):
                body = body.encode('utf-8')
            request_bytes = request.encode('utf-8') + body
        else:
            request_bytes = request.encode('utf-8')
        send_all(ssl_sock, request_bytes)
        response = b''
        while True:
            chunk = ssl_sock.recv(4096)
            if not chunk:
                break
            response += chunk
        header_end = response.find(b'\r\n\r\n')
        if header_end == -1:
            return 0, {}, response
        header_bytes = response[:header_end]
        body_bytes = response[header_end + 4:]
        header_lines = header_bytes.decode('utf-8', errors='replace').split('\r\n')
        status_line = header_lines[0]
        status_code = int(status_line.split(' ')[1]) if len(status_line.split(' ')) > 1 else 0
        response_headers = {}
        for line in header_lines[1:]:
            if ': ' in line:
                key, value = line.split(': ', 1)
                response_headers[key] = value
        return status_code, response_headers, body_bytes
    finally:
        close_connection(ssl_sock)