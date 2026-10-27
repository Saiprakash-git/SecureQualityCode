import argparse
import json
import sys
from ldap3 import ALL, ALL_ATTRIBUTES, Connection, Server, SUBTREE
from ldap3.utils.conv import escape_filter_chars


def domain_to_base_dn(domain: str) -> str:
    """
    Converts a domain string into an LDAP Base DN.
    If the string is already in 'dc=...,dc=...' format, returns it as-is.
    Otherwise, splits standard domain names (e.g., 'example.com') into DC components.
    """
    if "dc=" in domain.lower():
        return domain
    parts = [part.strip() for part in domain.split(".") if part.strip()]
    return ",".join(f"dc={part}" for part in parts)


def query_ldap_user(
    host: str,
    port: int,
    base_dn: str,
    username: str
) -> list:
    """
    Connects to the LDAP server, constructs a safe search filter using the
    provided username, and queries for matching user entries.
    """
    server = Server(host, port=port, get_info=ALL)
    with Connection(server, auto_bind=True) as conn:
        safe_username = escape_filter_chars(username)
        search_filter = f"(|(uid={safe_username})(sAMAccountName={safe_username})(cn={safe_username}))"

        conn.search(
            search_base=base_dn,
            search_filter=search_filter,
            search_scope=SUBTREE,
            attributes=ALL_ATTRIBUTES
        )

        user_records = []
        for entry in conn.entries:
            user_records.append(json.loads(entry.entry_to_json()))

        return user_records


def parse_arguments(argv: list) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Extract domain and username to search an LDAP server on localhost."
    )
    parser.add_argument(
        "username",
        type=str,
        help="Username to query in the LDAP directory"
    )
    parser.add_argument(
        "domain",
        type=str,
        help="Domain name (e.g., example.com) or Base DN (e.g., dc=example,dc=com)"
    )
    parser.add_argument(
        "--host",
        type=str,
        default="localhost",
        help="LDAP server hostname or IP address (default: localhost)"
    )
    parser.add_argument(
        "--port",
        type=int,
        default=389,
        help="LDAP server port (default: 389)"
    )
    return parser.parse_args(argv)


def main(argv: list = None) -> int:
    if argv is None:
        argv = sys.argv[1:]

    args = parse_arguments(argv)
    base_dn = domain_to_base_dn(args.domain)

    try:
        results = query_ldap_user(
            host=args.host,
            port=args.port,
            base_dn=base_dn,
            username=args.username
        )
        print(json.dumps(results, indent=2))
        return 0
    except Exception as exc:
        print(f"Error querying LDAP server: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())