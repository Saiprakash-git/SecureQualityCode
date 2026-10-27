import argparse
import socket
import struct
import sys


def parse_dns_question(packet):
    """Validate a DNS query and return its first question and header fields."""
    if len(packet) < 12:
        raise ValueError("DNS packet is shorter than its header")

    transaction_id, flags, question_count, _, _, _ = struct.unpack("!HHHHHH", packet[:12])
    if flags & 0x8000:
        raise ValueError("Expected a DNS query, received a response")
    if question_count == 0:
        raise ValueError("DNS query contains no questions")

    labels = []
    position = 12
    question_end = None
    visited = set()

    while True:
        if position >= len(packet):
            raise ValueError("DNS question name is truncated")

        length = packet[position]

        if length & 0xC0 == 0xC0:
            if position + 1 >= len(packet):
                raise ValueError("Truncated DNS compression pointer")
            pointer = ((length & 0x3F) << 8) | packet[position + 1]
            if pointer >= len(packet) or pointer in visited:
                raise ValueError("Invalid DNS compression pointer")
            visited.add(pointer)
            if question_end is None:
                question_end = position + 2
            position = pointer
            continue

        if length & 0xC0:
            raise ValueError("Invalid DNS label encoding")

        position += 1
        if length == 0:
            if question_end is None:
                question_end = position
            break

        if length > 63 or position + length > len(packet):
            raise ValueError("Invalid or truncated DNS label")

        labels.append(packet[position:position + length].decode("ascii", errors="replace"))
        position += length

    if question_end + 4 > len(packet):
        raise ValueError("DNS question is missing its type or class")

    query_type, query_class = struct.unpack("!HH", packet[question_end:question_end + 4])
    question_end += 4

    return {
        "transaction_id": transaction_id,
        "flags": flags,
        "name": ".".join(labels),
        "type": query_type,
        "class": query_class,
        "question": packet[12:question_end],
    }


def make_error_response(packet, rcode, question=None):
    """Build a DNS error response, including the question when it was parsed."""
    if len(packet) >= 2:
        transaction_id = struct.unpack("!H", packet[:2])[0]
    else:
        transaction_id = 0

    request_flags = struct.unpack("!H", packet[2:4])[0] if len(packet) >= 4 else 0
    flags = (
        0x8000
        | (request_flags & 0x7800)
        | (request_flags & 0x0100)
        | 0x0080
        | (request_flags & 0x0010)
        | (rcode & 0x000F)
    )

    question_data = question["question"] if question else b""
    question_count = 1 if question else 0
    header = struct.pack("!HHHHHH", transaction_id, flags, question_count, 0, 0, 0)
    return header + question_data


def resolve_dns_query(packet, upstream_ip, upstream_port, timeout):
    """Forward a validated DNS query to an upstream resolver and return its response."""
    question = parse_dns_question(packet)

    addresses = socket.getaddrinfo(
        upstream_ip, upstream_port, type=socket.SOCK_DGRAM
    )
    if not addresses:
        raise OSError("Could not resolve the upstream DNS server")

    family, socktype, protocol, _, upstream_address = addresses[0]
    with socket.socket(family, socktype, protocol) as upstream_socket:
        upstream_socket.settimeout(timeout)
        upstream_socket.connect(upstream_address)
        upstream_socket.send(packet)
        response = upstream_socket.recv(65535)

    if len(response) < 12:
        raise OSError("Upstream DNS response is shorter than its header")

    response_id, response_flags = struct.unpack("!HH", response[:4])
    if response_id != question["transaction_id"] or not response_flags & 0x8000:
        raise OSError("Received an invalid DNS response from the upstream server")

    return response


def serve_dns(listen_ip, listen_port, upstream_ip="8.8.8.8", upstream_port=53, timeout=5.0):
    """Listen for DNS queries, forward them to an upstream resolver, and relay replies."""
    addresses = socket.getaddrinfo(
        listen_ip, listen_port, type=socket.SOCK_DGRAM, flags=socket.AI_PASSIVE
    )
    if not addresses:
        raise OSError("Could not determine a listening address")

    family, socktype, protocol, _, listen_address = addresses[0]
    with socket.socket(family, socktype, protocol) as server_socket:
        server_socket.bind(listen_address)
        print(
            f"Listening for DNS requests on {listen_ip}:{listen_port}; "
            f"upstream resolver is {upstream_ip}:{upstream_port}",
            file=sys.stderr,
        )

        while True:
            packet, client_address = server_socket.recvfrom(65535)

            try:
                response = resolve_dns_query(
                    packet, upstream_ip, upstream_port, timeout
                )
            except ValueError:
                response = make_error_response(packet, 1)
            except (OSError, socket.timeout):
                try:
                    question = parse_dns_question(packet)
                except ValueError:
                    question = None
                response = make_error_response(packet, 2, question)

            try:
                server_socket.sendto(response, client_address)
            except OSError as error:
                print(f"Could not send DNS response to {client_address}: {error}", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(
        description="UDP DNS proxy that forwards requests to an upstream resolver."
    )
    parser.add_argument("listen_ip", help="IP address on which to listen")
    parser.add_argument("port", type=int, help="UDP port on which to listen")
    parser.add_argument("--upstream-ip", default="8.8.8.8", help="Upstream DNS server IP or hostname")
    parser.add_argument("--upstream-port", type=int, default=53, help="Upstream DNS server UDP port")
    parser.add_argument("--timeout", type=float, default=5.0, help="Upstream response timeout in seconds")
    args = parser.parse_args()

    serve_dns(
        args.listen_ip,
        args.port,
        args.upstream_ip,
        args.upstream_port,
        args.timeout,
    )


if __name__ == "__main__":
    main()