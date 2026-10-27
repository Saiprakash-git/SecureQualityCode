#!/usr/bin/env python3
import sys
import json
from ldap3 import Server, Connection, ALL, SUBTREE
from ldap3.utils.conv import escape_filter_chars


def parse_request(argv):
    args = argv[1:]
    if not args:
        raise ValueError("Usage: script.py <dc> <username>  OR  script.py <username@domain>")
    if len(args) >= 2:
        dc_value = args[0]
        username = args[1]
    else:
        if "@" not in args[0]:
            raise ValueError("Single argument must be in the form username@domain")
        username, dc_value = args[0].split("@", 1)
    if not username or not dc_value:
        raise ValueError("Both dc and username must be provided")
    return dc_value.strip(), username.strip()


def build_base_dn(dc_value):
    if "=" in dc_value:
        return dc_value
    parts = [p for p in dc_value.split(".") if p]
    return ",".join(f"dc={p}" for p in parts)


def build_search_filter(username):
    return f"(&(objectClass=person)(uid={escape_filter_chars(username)}))"


def search_user(base_dn, search_filter, host="localhost", port=389,
                bind_dn=None, password=None):
    server = Server(host, port=port, get_info=ALL)
    conn = Connection(server, user=bind_dn, password=password, auto_bind=True)
    try:
        conn.search(
            search_base=base_dn,
            search_filter=search_filter,
            search_scope=SUBTREE,
            attributes=["*"]
        )
        results = []
        for entry in conn.entries:
            record = {"dn": entry.entry_dn}
            record.update(entry.entry_attributes_as_dict)
            results.append(record)
        return results
    finally:
        conn.unbind()


def main(argv):
    try:
        dc_value, username = parse_request(argv)
        base_dn = build_base_dn(dc_value)
        search_filter = build_search_filter(username)
        users = search_user(base_dn, search_filter)
        print(json.dumps(users, indent=2, default=str))
        return 0
    except Exception as exc:
        print(json.dumps({"error": str(exc)}), file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))