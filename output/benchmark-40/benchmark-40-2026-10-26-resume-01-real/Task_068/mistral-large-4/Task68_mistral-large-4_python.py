import sys
from ldap3 import Server, Connection, ALL, SUBTREE

def construct_base_dn(dc):
    if dc.startswith('dc='):
        return dc
    parts = dc.split('.')
    return ','.join(['dc=' + part for part in parts])

def construct_ldap_filter(username):
    return '(uid=' + username + ')'

def connect_ldap(host='localhost'):
    server = Server(host, get_info=ALL)
    conn = Connection(server, auto_bind=True)
    return conn

def search_ldap_user(dc, username):
    base_dn = construct_base_dn(dc)
    search_filter = construct_ldap_filter(username)

    conn = connect_ldap('localhost')

    conn.search(
        search_base=base_dn,
        search_filter=search_filter,
        search_scope=SUBTREE,
        attributes=['*']
    )

    results = []
    for entry in conn.entries:
        results.append(entry.entry_attributes)

    conn.unbind()
    return results

def main():
    if len(sys.argv) < 3:
        return "Usage: python script.py <dc> <username>"

    dc = sys.argv[1]
    username = sys.argv[2]

    return search_ldap_user(dc, username)

if __name__ == '__main__':
    result = main()
    print(result)