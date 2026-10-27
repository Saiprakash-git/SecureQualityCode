import javax.naming.*;
import javax.naming.directory.*;
import java.util.*;

public class LdapUserExtractor {

    public static void main(String[] args) {
        if (args.length < 2) {
            System.err.println("Usage: java LdapUserExtractor <username> <domain>");
            System.exit(1);
        }

        String username = args[0];
        String domain = args[1];

        String userInfo = extractUserInfo(username, domain);
        System.out.println(userInfo);
    }

    public static String extractUserInfo(String username, String domain) {
        String baseDN = constructBaseDN(domain);
        String searchFilter = constructSearchFilter(username);

        Hashtable<String, String> env = new Hashtable<>();
        env.put(Context.INITIAL_CONTEXT_FACTORY, "com.sun.jndi.ldap.LdapCtxFactory");
        env.put(Context.PROVIDER_URL, "ldap://localhost:389");

        DirContext ctx = null;
        StringBuilder result = new StringBuilder();

        try {
            ctx = new InitialDirContext(env);

            SearchControls controls = new SearchControls();
            controls.setSearchScope(SearchControls.SUBTREE_SCOPE);

            NamingEnumeration<SearchResult> results = ctx.search(baseDN, searchFilter, controls);

            boolean found = false;
            while (results.hasMore()) {
                found = true;
                SearchResult sr = results.next();
                result.append(formatSearchResult(sr));
            }

            if (!found) {
                result.append("User not found: ").append(username);
            }

        } catch (NamingException e) {
            result.append("LDAP Error: ").append(e.toString());
        } finally {
            if (ctx != null) {
                try {
                    ctx.close();
                } catch (NamingException e) {
                }
            }
        }

        return result.toString();
    }

    public static String constructBaseDN(String domain) {
        if (domain == null || domain.isEmpty()) {
            return "";
        }
        if (domain.toLowerCase().startsWith("dc=")) {
            return domain;
        }
        String[] parts = domain.split("\\.");
        StringBuilder dn = new StringBuilder();
        for (int i = 0; i < parts.length; i++) {
            if (i > 0) dn.append(",");
            dn.append("dc=").append(parts[i].trim());
        }
        return dn.toString();
    }

    public static String constructSearchFilter(String username) {
        return "(uid=" + escapeLDAPSearchFilter(username) + ")";
    }

    private static String escapeLDAPSearchFilter(String input) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < input.length(); i++) {
            char c = input.charAt(i);
            switch (c) {
                case '\\':
                    sb.append("\\5c");
                    break;
                case '*':
                    sb.append("\\2a");
                    break;
                case '(':
                    sb.append("\\28");
                    break;
                case ')':
                    sb.append("\\29");
                    break;
                case '\u0000':
                    sb.append("\\00");
                    break;
                default:
                    sb.append(c);
            }
        }
        return sb.toString();
    }

    private static String formatSearchResult(SearchResult sr) throws NamingException {
        StringBuilder sb = new StringBuilder();
        sb.append("DN: ").append(sr.get