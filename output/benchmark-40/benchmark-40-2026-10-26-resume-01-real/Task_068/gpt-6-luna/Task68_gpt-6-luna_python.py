import argparse
import json
import re
import sys


def _ldap_filter_escape(value):
    return (
        value.replace("\\", r"\5c")
        .replace("*", r"\2a")
        .replace("(", r"\28")
        .replace(")", r"\29")
        .replace("\x00", r"\00")
    )


def _unescape_dn_value(value):
    return re.sub(
        r"\\([0-9a-fA-F]{2}|.)",
        lambda match: (
            bytes.fromhex(match.group(1)).decode("utf-8", errors="replace")
            if len(match.group(1)) == 2
            and all(char in "0123456789abcdefABCDEF" for char in match.group(1))
            else match.group(1)
        ),
        value,
    )


def extract_domain_and_username(request):
    """Extract a username and domain components from a DN or user principal name."""
    if not isinstance(request, str) or not request.strip():
        raise ValueError("Request must be a non-empty string")

    request = request.strip()

    # Handle LDAP distinguished names such as:
    # uid=alice,dc=example,dc=com
    # CN=Alice Smith,OU=People,DC=example,DC=com
    attributes = []
    for part in re.split(r"(?<!\\),", request):
        match = re.match(r"\s*([a-zA-Z][a-zA-Z0-9-]*)\s*=\s*(.*?)\s*$", part)
        if match:
            attributes.append((match.group(1).lower(), _unescape_dn_value(match.group(2))))

    domain_parts = [value for key, value in attributes if key == "dc"]
    username = next(
        (
            value
            for key, value in attributes
            if key in {"uid", "samaccountname", "user", "username"}
        ),
        None,
    )

    if username is None:
        username = next(
            (value for key, value in attributes if key in {"cn", "name"}),
            None,
        )

    if domain_parts:
        if not username:
            raise ValueError("No username was found in the request")
        return username, domain_parts

    # Handle user principal names such as alice@example.com.
    if "@" in request:
        username, domain = request.rsplit("@", 1)
        domain_parts = [part for part in domain.split(".") if part]
        if not username or not domain_parts:
            raise ValueError("Invalid user principal name")
        return username, domain_parts

    raise ValueError(
        "Request must contain a distinguished name with dc components "
        "or a user principal name such as alice@example.com"
    )


def build_ldap_query(username):
    escaped = _ldap_filter_escape(username)
    return (
        "(&(objectClass=person)"
        "(|"
        f"(uid={escaped})"
        f"(sAMAccountName={escaped})"
        f"(cn={escaped})"
        "))"
    )


def search_user(request, host="localhost", port=389):
    """Search LDAP for the user described by request and return matching entries."""
    try:
        from ldap3 import ALL, SUBTREE, Connection, Server
    except ImportError as exc:
        raise RuntimeError("The ldap3 package is required (pip install ldap3)") from exc

    username, domain_parts = extract_domain_and_username(request)
    search_base = ",".join(f"dc={part}" for part in domain_parts)
    search_filter = build_ldap_query(username)

    server = Server(host, port=port, get_info=ALL)
    connection = Connection(server)

    try:
        if not connection.bind():
            raise RuntimeError(f"LDAP bind failed: {connection.result}")

        if not connection.search(
            search_base=search_base,
            search_filter=search_filter,
            search_scope=SUBTREE,
            attributes=["*"],
        ):
            if connection.result and connection.result.get("description") not in {
                "success",
                "noSuchObject",
            }:
                raise RuntimeError(f"LDAP search failed: {connection.result}")

        return [
            {
                "dn": entry.entry_dn,
                "attributes": entry.entry_attributes_as_dict,
            }
            for entry in connection.entries
        ]
    finally:
        connection.unbind()


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Search a local LDAP server using a username and domain."
    )
    parser.add_argument(
        "request",
        help="A user principal name (alice@example.com) or LDAP DN",
    )
    parser.add_argument("--host", default="localhost")
    parser.add_argument("--port", type=int, default=389)
    args = parser.parse_args(argv)

    try:
        results = search_user(args.request, host=args.host, port=args.port)
    except Exception as exc:
        print(json.dumps({"error": str(exc)}))
        return 1

    print(json.dumps(results, default=str))
    return 0


if __name__ == "__main__":
    sys.exit(main())