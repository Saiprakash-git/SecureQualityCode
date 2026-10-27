import socket
import struct
import sys

def parse_dns_header(data):
    if len(data) < 12:
        return None
    header_id, flags, qdcount, ancount, nscount, arcount = struct.unpack('!HHHHHH', data[:12])
    return {
        'id': header_id,
        'flags': flags,
        'qdcount': qdcount,
        'ancount': ancount,
        'nscount': nscount,
        'arcount': arcount
    }

def parse_dns_name(data, offset):
    labels = []
    jumped = False
    original_offset = offset
    while True:
        if offset >= len(data):
            break
        length = data[offset]
        if length == 0:
            offset += 1
            if not jumped:
                original_offset = offset
            break
        if length & 0xC0 == 0xC0:
            pointer = struct.unpack('!H', data[offset:offset+2])[0] & 0x3FFF
            if not jumped:
                original_offset = offset + 2
            offset = pointer
            jumped = True
            continue
        offset += 1
        labels.append(data[offset:offset+length].decode('utf-8', errors='ignore'))
        offset += length
    return '.'.join(labels), original_offset

def parse_dns_question(data, offset):
    qname, offset = parse_dns_name(data, offset)
    if offset + 4 > len(data):
        return None, offset
    qtype, qclass = struct.unpack('!HH', data[offset:offset+4])
    offset += 4
    return {
        'name': qname,
        'type': qtype,
        'class': qclass
    }, offset

def encode_dns_name(name):
    parts = name.split('.')
    result = b''
    for part in parts:
        if part:
            result += bytes([len(part)]) + part.encode('utf-8')
    result += b'\x00'
    return result

def build_dns_response(query_id, question, answers):
    flags = 0x8180
    header = struct.pack('!HHHHHH', query_id, flags, 1, len(answers), 0, 0)

    qname_encoded = encode_dns_name(question['name'])
    question_section = qname_encoded + struct.pack('!HH', question['type'], question['class'])

    answer_section = b''
    for answer in answers:
        answer_section += encode_dns_name(question['name'])
        answer_section += struct.pack('!HHIH', answer['type'], answer['class'], answer['ttl'], len(answer['data']))
        answer_section += answer['data']

    return header + question_section + answer_section

def resolve_dns_record(query_name, query_type):
    answers = []
    if query_type == 1:
        try:
            ip = socket.gethostbyname(query_name)
            ip_bytes = socket.inet_aton(ip)
            answers.append({
                'type': 1,
                'class': 1,
                'ttl': 300,
                'data': ip_bytes
            })
        except socket.gaierror:
            pass
    return answers

def handle_dns_query(data):
    header = parse_dns_header(data)
    if not header:
        return None

    question, offset = parse_dns_question(data, 12)
    if not question:
        return None

    answers = resolve_dns_record(question['name'], question['type'])
    response = build_dns_response(header['id'], question, answers)
    return response

def start_dns_server(ip_address, port):
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind((ip_address, port))

    while True:
        try:
            data, addr = sock.recvfrom(512)
            response = handle_dns_query(data)
            if response:
                sock.sendto(response, addr)
        except Exception:
            continue

if __name__ == '__main__':
    ip = sys.argv[1] if len(sys.argv) > 1 else '0.0.0.0'
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 5353
    start_dns_server(ip, port)