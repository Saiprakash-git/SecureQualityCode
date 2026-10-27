import java.util.Hashtable;
import javax.naming.Context;
import javax.naming.NamingEnumeration;
import javax.naming.NamingException;
import javax.naming.directory.Attribute;
import javax.naming.directory.Attributes;
import javax.naming.directory.DirContext;
import javax.naming.directory.InitialDirContext;
import javax.naming.directory.SearchControls;
import javax.naming.directory.SearchResult;

public class LdapUserSearch {

    public static void main(String[] args) {
        if (args.length < 2) {
            System.err.println("Usage: java LdapUserSearch <domain_component> <username>");
            System.exit(1);
        }

        String dcInput = args[0];
        String username = args[1];
        String host = "localhost";
        int port = 389;

        String userInfo = searchUser(host, port, dcInput, username);
        System.out.println(userInfo);
    }

    public static String searchUser(String host, int port, String domainInput, String username) {
        String baseDn = buildBaseDn(domainInput);
        String filter = buildLdapFilter(username);

        Hashtable<String, String> env = new Hashtable<>();
        env.put(Context.INITIAL_CONTEXT_FACTORY, "com.sun.jndi.ldap.LdapCtxFactory");
        env.put(Context.PROVIDER_URL, "ldap://" + host + ":" + port);
        env.put(Context.SECURITY_AUTHENTICATION, "none");

        DirContext ctx = null;
        StringBuilder resultBuilder = new StringBuilder();

        try {
            ctx = new InitialDirContext(env);
            SearchControls controls = new SearchControls();
            controls.setSearchScope(SearchControls.SUBTREE_SCOPE);

            NamingEnumeration<SearchResult> results = ctx.search(baseDn, filter, controls);

            boolean found = false;
            while (results != null && results.hasMore()) {
                found = true;
                SearchResult sr = results.next();
                resultBuilder.append("DN: ").append(sr.getNameInNamespace()).append("\n");
                
                Attributes attrs = sr.getAttributes();
                NamingEnumeration<? extends Attribute> attrEnum = attrs.getAll();
                while (attrEnum.hasMore()) {
                    Attribute attr = attrEnum.next();
                    resultBuilder.append("  ").append(attr.getID()).append(": ");
                    NamingEnumeration<?> values = attr.getAll();
                    while (values.hasMore()) {
                        resultBuilder.append(values.next());
                        if (values.hasMore()) {
                            resultBuilder.append(", ");
                        }
                    }
                    resultBuilder.append("\n");
                }
                resultBuilder.append("\n");
            }

            if (!found) {
                return "No user found matching username '" + username + "' in base DN '" + baseDn + "'.";
            }

        } catch (NamingException e) {
            return "LDAP Error: " + e.getMessage();
        } finally {
            if (ctx != null) {
                try {
                    ctx.close();
                } catch (NamingException ignored) {
                }
            }
        }

        return resultBuilder.toString().trim();
    }

    public static String buildBaseDn(String domain) {
        if (domain == null || domain.trim().isEmpty()) {
            return "";
        }
        String clean = domain.trim();
        if (clean.toLowerCase().contains("dc=")) {
            return clean;
        }
        String[] parts = clean.split("\\.");
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < parts.length; i++) {
            if (!parts[i].isEmpty()) {
                sb.append("dc=").append(parts[i]);
                if (i < parts.length - 1) {
                    sb.append(",");
                }
            }
        }
        return sb.toString();
    }

    public static String buildLdapFilter(String username) {
        String sanitizedUser = escapeLdapFilter(username);
        return "(|(uid=" + sanitizedUser + ")(sAMAccountName=" + sanitizedUser + ")(cn=" + sanitizedUser + "))";
    }

    public static String escapeLdapFilter(String filter) {
        if (filter == null) {
            return "";
        }
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < filter.length(); i++) {
            char ch = filter.charAt(i);
            switch (ch) {
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
                case '\0':
                    sb.append("\\00");
                    break;
                default:
                    sb.append(ch);
            }
        }
        return sb.toString();
    }
}