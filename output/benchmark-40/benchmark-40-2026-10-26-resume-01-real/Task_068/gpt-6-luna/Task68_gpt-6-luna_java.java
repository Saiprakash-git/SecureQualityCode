import javax.naming.Context;
import javax.naming.NamingEnumeration;
import javax.naming.NamingException;
import javax.naming.directory.Attributes;
import javax.naming.directory.DirContext;
import javax.naming.directory.InitialDirContext;
import javax.naming.directory.SearchControls;
import javax.naming.directory.SearchResult;
import javax.naming.ldap.LdapName;
import javax.naming.ldap.Rdn;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Hashtable;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.TreeMap;

public class Main {
    private static final String LDAP_URL = "ldap://localhost:389";

    private static final class UserRequest {
        private final String username;
        private final String baseDn;

        private UserRequest(String username, String baseDn) {
            this.username = username;
            this.baseDn = baseDn;
        }
    }

    public static void main(String[] args) {
        if (args.length == 0) {
            System.err.println("Usage: java Main <username@domain | user-DN> [domain]");
            System.exit(2);
        }

        try {
            UserRequest request = args.length >= 2
                    ? fromUsernameAndDomain(args[0], args[1])
                    : parseRequest(args[0]);
            System.out.print(lookupUser(request));
        } catch (Exception e) {
            System.err.println("LDAP lookup failed: " + e.getMessage());
            System.exit(1);
        }
    }

    private static UserRequest parseRequest(String input) throws Exception {
        if (input == null || input.trim().isEmpty()) {
            throw new IllegalArgumentException("The request must not be empty.");
        }

        String value = input.trim();
        if (value.toLowerCase(Locale.ROOT).contains("dc=")) {
            return parseDistinguishedName(value);
        }

        int at = value.lastIndexOf('@');
        if (at > 0 && at < value.length() - 1) {
            return fromUsernameAndDomain(value.substring(0, at), value.substring(at + 1));
        }

        int slash = value.lastIndexOf('\\');
        if (slash > 0 && slash < value.length() - 1) {
            return fromUsernameAndDomain(value.substring(slash + 1), value.substring(0, slash));
        }

        throw new IllegalArgumentException(
                "Provide a user DN, username@domain, or username and domain as separate arguments.");
    }

    private static UserRequest parseDistinguishedName(String distinguishedName) throws Exception {
        LdapName name = new LdapName(distinguishedName);
        String username = null;
        List<String> domainComponents = new ArrayList<>();

        for (Rdn rdn : name.getRdns()) {
            String type = rdn.getType().toLowerCase(Locale.ROOT);
            String value = String.valueOf(rdn.getValue());
            if (type.equals("dc")) {
                domainComponents.add(value);
            } else if (username == null
                    && (type.equals("uid") || type.equals("samaccountname")
                    || type.equals("cn") || type.equals("user"))) {
                username = value;
            }
        }

        if (username == null || domainComponents.isEmpty()) {
            throw new IllegalArgumentException("The DN must contain a username attribute and at least one dc component.");
        }

        Collections.reverse(domainComponents);
        return new UserRequest(username, toBaseDn(domainComponents));
    }

    private static UserRequest fromUsernameAndDomain(String username, String domain) {
        if (username == null || username.trim().isEmpty()) {
            throw new IllegalArgumentException("The username must not be empty.");
        }
        if (domain == null || domain.trim().isEmpty()) {
            throw new IllegalArgumentException("The domain must not be empty.");
        }

        String cleanDomain = domain.trim();
        List<String> components = new ArrayList<>();

        if (cleanDomain.toLowerCase(Locale.ROOT).contains("dc=")) {
            try {
                LdapName name = new LdapName(cleanDomain);
                for (Rdn rdn : name.getRdns()) {
                    if (rdn.getType().equalsIgnoreCase("dc")) {
                        components.add(String.valueOf(rdn.getValue()));
                    }
                }
                Collections.reverse(components);
            } catch (NamingException e) {
                throw new IllegalArgumentException("Invalid domain DN: " + domain, e);
            }
        } else {
            String normalized = cleanDomain.replace(',', '.');
            for (String component : normalized.split("\\.")) {
                String part = component.trim();
                if (!part.isEmpty()) {
                    components.add(part);
                }
            }
        }

        if (components.isEmpty()) {
            throw new IllegalArgumentException("The domain must contain at least one component.");
        }

        return new UserRequest(username.trim(), toBaseDn(components));
    }

    private static String toBaseDn(List<String> domainComponents) {
        StringBuilder baseDn = new StringBuilder();
        for (String component : domainComponents) {
            if (baseDn.length() > 0) {
                baseDn.append(',');
            }
            baseDn.append("dc=").append(escapeDnValue(component));
        }
        return baseDn.toString();
    }

    private static String escapeDnValue(String value) {
        try {
            return Rdn.escapeValue(value).toString();
        } catch (RuntimeException e) {
            throw new IllegalArgumentException("Invalid domain component: " + value, e);
        }
    }

    private static String lookupUser(UserRequest request) throws NamingException {
        String filter = "(uid=" + escapeLdapFilterValue(request.username) + ")";
        Hashtable<String, String> environment = new Hashtable<>();
        environment.put(Context.INITIAL_CONTEXT_FACTORY, "com.sun.jndi.ldap.LdapCtxFactory");
        environment.put(Context.PROVIDER_URL, LDAP_URL);
        environment.put(Context.REFERRAL, "ignore");

        DirContext context = null;
        try {
            context = new InitialDirContext(environment);
            SearchControls controls = new SearchControls();
            controls.setSearchScope(SearchControls.SUBTREE_SCOPE);

            NamingEnumeration<SearchResult> results = context.search(request.baseDn, filter, controls);
            StringBuilder output = new StringBuilder();
            try {
                while (results.hasMore()) {
                    SearchResult result = results.next();
                    String dn;
                    try {
                        dn = result.getNameInNamespace();
                    } catch (UnsupportedOperationException e) {
                        dn = result.getName() + "," + request.baseDn;
                    }

                    output.append("dn: ").append(dn).append('\n');
                    appendAttributes(output, result.getAttributes());
                    output.append('\n');
                }
            } finally {
                results.close();
            }

            if (output.length() == 0) {
                return "No user found for " + request.username + " in " + request.baseDn + System.lineSeparator();
            }
            return output.toString();
        } finally {
            if (context != null) {
                context.close();
            }
        }
    }

    private static void appendAttributes(StringBuilder output, Attributes attributes) throws NamingException {
        NamingEnumeration<? extends javax.naming.directory.Attribute> all = attributes.getAll();
        try {
            while (all.hasMore()) {
                javax.naming.directory.Attribute attribute = all.next();
                NamingEnumeration<?> values = attribute.getAll();
                try {
                    while (values.hasMore()) {
                        output.append(attribute.getID())
                                .append(": ")
                                .append(String.valueOf(values.next()))
                                .append('\n');
                    }
                } finally {
                    values.close();
                }
            }
        } finally {
            all.close();
        }
    }

    private static String escapeLdapFilterValue(String value) {
        StringBuilder escaped = new StringBuilder();
        for (int i = 0; i < value.length(); i++) {
            char c = value.charAt(i);
            switch (c) {
                case '\\':
                    escaped.append("\\5c");
                    break;
                case '*':
                    escaped.append("\\2a");
                    break;
                case '(':
                    escaped.append("\\28");
                    break;
                case ')':
                    escaped.append("\\29");
                    break;
                case '\0':
                    escaped.append("\\00");
                    break;
                default:
                    escaped.append(c);
            }
        }
        return escaped.toString();
    }
}