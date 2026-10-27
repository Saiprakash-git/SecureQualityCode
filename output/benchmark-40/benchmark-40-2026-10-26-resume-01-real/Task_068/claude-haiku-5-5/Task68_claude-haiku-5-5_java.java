import javax.naming.Context;
import javax.naming.NamingEnumeration;
import javax.naming.NamingException;
import javax.naming.directory.Attribute;
import javax.naming.directory.Attributes;
import javax.naming.directory.DirContext;
import javax.naming.directory.InitialDirContext;
import javax.naming.directory.SearchControls;
import javax.naming.directory.SearchResult;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Hashtable;
import java.util.regex.Pattern;

public class LdapUserLookup {

    private static final String LDAP_URL = "ldap://localhost:389";
    private static final Pattern DC_PATTERN = Pattern.compile("^[A-Za-z0-9-]+$");

    public static void main(String[] args) {
        try {
            Map<String, String> request = parseRequest(args);
            String username = request.get("username");
            String dcInput = request.get("dc");

            String baseDn = buildBaseDn(dcInput);
            String filter = buildUserFilter(username);

            String userInfo = searchUser(baseDn, filter);
            System.out.println(userInfo);
        } catch (IllegalArgumentException | NamingException e) {
            System.err.println("Error: " + e.getMessage());
            System.exit(1);
        }
    }

    static Map<String, String> parseRequest(String[] args) {
        if (args == null || args.length == 0) {
            throw new IllegalArgumentException("No input provided. Usage: <username> <dc> or username=<user> dc=<domain>");
        }

        Map<String, String> result = new HashMap<>();
        StringBuilder dcBuilder = new StringBuilder();

        for (String arg : args) {
            if (arg == null) {
                continue;
            }
            for (String part : arg.split("[&;\\s]+")) {
                part = part.trim();
                if (part.isEmpty()) {
                    continue;
                }
                int eq = part.indexOf('=');
                if (eq > 0) {
                    String key = part.substring(0, eq).trim().toLowerCase();
                    String value = part.substring(eq + 1).trim();
                    if (key.equals("dc") || key.equals("domain")) {
                        if (dcBuilder.length() > 0) {
                            dcBuilder.append(',');
                        }
                        dcBuilder.append(value);
                    } else if (key.equals("user") || key.equals("username") || key.equals("uid")) {
                        result.put("username", value);
                    }
                } else if (!result.containsKey("username")) {
                    result.put("username", part);
                } else {
                    if (dcBuilder.length() > 0) {
                        dcBuilder.append(',');
                    }
                    dcBuilder.append(part);
                }
            }
        }

        String username = result.get("username");
        if (username == null || username.isEmpty()) {
            throw new IllegalArgumentException("Username could not be extracted from input.");
        }

        if (username.contains("@")) {
            int at = username.indexOf('@');
            String domain = username.substring(at + 1);
            username = username.substring(0, at);
            result.put("username", username);
            if (dcBuilder.length() == 0) {
                dcBuilder.append(domain);
            }
        }

        if (dcBuilder.length() == 0) {
            throw new IllegalArgumentException("Domain component (dc) could not be extracted from input.");
        }

        result.put("dc", dcBuilder.toString());
        return result;
    }

    static String buildBaseDn(String dcInput) {
        if (dcInput == null || dcInput.isEmpty()) {
            throw new IllegalArgumentException("Domain component (dc) is empty.");
        }

        List<String> components = new ArrayList<>();
        String normalized = dcInput.replaceAll("(?i)dc\\s*=", "");
        for (String token : normalized.split("[,.]")) {
            String dc = token.trim();
            if (dc.isEmpty()) {
                continue;
            }
            if (!DC_PATTERN.matcher(dc).matches()) {
                throw new IllegalArgumentException("Invalid domain component: " + dc);
            }
            components.add("dc=" + dc);
        }

        if (components.isEmpty()) {
            throw new IllegalArgumentException("No valid domain components found.");
        }
        return String.join(",", components);
    }

    static String buildUserFilter(String username) {
        if (username == null || username.isEmpty()) {
            throw new IllegalArgumentException("Username is empty.");
        }
        String escaped = escapeFilterValue(username);
        return "(|(uid=" + escaped + ")(sAMAccountName=" + escaped + ")(cn=" + escaped + "))";
    }

    static String escapeFilterValue(String value) {
        StringBuilder sb = new StringBuilder();
        for (char c : value.toCharArray()) {
            switch (c) {
                case '\\': sb.append("\\5c"); break;
                case '*':  sb.append("\\2a"); break;
                case '(':  sb.append("\\28"); break;
                case ')':  sb.append("\\29"); break;
                case '\0': sb.append("\\00"); break;
                default:   sb.append(c);
            }
        }
        return sb.toString();
    }

    static String searchUser(String baseDn, String filter) throws NamingException {
        Hashtable<String, Object> env = new Hashtable<>();
        env.put(Context.INITIAL_CONTEXT_FACTORY, "com.sun.jndi.ldap.LdapCtxFactory");
        env.put(Context.PROVIDER_URL, LDAP_URL);
        env.put(Context.SECURITY_AUTHENTICATION, "none");

        DirContext ctx = new InitialDirContext(env);
        try {
            SearchControls controls = new SearchControls();
            controls.setSearchScope(SearchControls.SUBTREE_SC