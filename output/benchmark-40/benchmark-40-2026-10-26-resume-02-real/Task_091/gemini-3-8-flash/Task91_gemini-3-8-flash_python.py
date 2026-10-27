import socket
import struct
from typing import Tuple, Optional, Dict


def parse_domain_name(data: bytes, offset: int) -> Tuple[str, int]:
    labels = []
    current_offset = offset
    while True:
        length = data[current_offset]
        current_offset += 1
        if length == 0:
            break
        labels.append(data[current_offset:current_offset + length].decode("ascii", errors="replace"))
        current_offset += length
    domain_name = ".".join(labels)
    return domain_name, current_offset


def parse_dns_query(data: bytes) -> Tuple[int, str, int, int, bytes]:
    transaction_id, flags, qdcount, _, _, _ = struct.unpack("!HHHHHH", data[:12])
    domain_name, offset = parse_domain_name(data, 12)
    qtype, qclass = struct.unpack("!HH", data[offset:offset + 4])
    question_bytes = data[12:offset + 4]
    return transaction_id, domain_name, qtype, qclass, question_bytes


def resolve_dns_record(domain_name: str, qtype: int, records_table: Dict[str, str]) -> Optional[str]:
    if qtype == 1:
        return records_table.get(domain_name.lower())
    return None


def build_dns_response(
    transaction_id: int,
    question_bytes: bytes,
    resolved_ip: Optional[str]
) -> bytes:
    if resolved_ip:
        flags = 0x8180
        ancount = 1
    else:
        flags = 0x8183
        ancount = 0

    header = struct.pack("!HHHHHH", transaction_id, flags, 1, ancount, 0, 0)
    response = bytearray(header + question_bytes)

    if resolved_ip:
        name_pointer = 0xC00C
        record_type = 1
        record_class = 1
        ttl = 300
        data_length = 4
        ip_bytes = socket.inet_aton(resolved_ip)

        answer = struct.pack(
            "!HHIH",
            name_pointer,
            record_type,
            record_class,
            ttl
        ) + struct.pack("!H", data_length) + ip_bytes
        response.extend(answer)

    return bytes(response)


def run_dns_server(host: str, port: int, records_table: Dict[str, str]) -> None:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((host, port))

    try:
        while True:
            data, client_address = sock.recvfrom(512)
            if len(data) < 12:
                continue

            transaction_id, domain, qtype, qclass, question_bytes = parse_dns_query(data)
            resolved_ip = resolve_dns_record(domain, qtype, records_table)
            response = build_dns_response(transaction_id, question_bytes, resolved_ip)

            sock.sendto(response, client_address)
    except KeyboardInterrupt:
        pass
    finally:
        sock.close()


if __name__ == "__main__":
    local_records = {
        "example.com": "93.184.216.34",
        "localhost": "127.0.0.1",
        "myserver.local": "192.168.1.100"
    }
    run_dns_server("0.0.0.0", 5353, local_records)