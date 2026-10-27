public class StringOperations {

    public static String copy(String source) {
        if (source == null) {
            return null;
        }
        return new String(source.toCharArray());
    }

    public static String concatenate(String first, String second) {
        if (first == null) first = "";
        if (second == null) second = "";
        return first.concat(second);
    }

    public static String concatenateAll(String separator, String... parts) {
        if (parts == null || parts.length == 0) {
            return "";
        }
        if (separator == null) separator = "";
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < parts.length; i++) {
            if (i > 0) sb.append(separator);
            if (parts[i] != null) sb.append(parts[i]);
        }
        return sb.toString();
    }

    public static String reverse(String s) {
        if (s == null) return null;
        return new StringBuilder(s).reverse().toString();
    }

    public static String toUpperCase(String s) {
        if (s == null) return null;
        return s.toUpperCase();
    }

    public static String toLowerCase(String s) {
        if (s == null) return null;
        return s.toLowerCase();
    }

    public static String capitalize(String s) {
        if (s == null || s.isEmpty()) return s;
        return Character.toUpperCase(s.charAt(0)) + s.substring(1).toLowerCase();
    }

    public static String substring(String s, int start, int end) {
        if (s == null) return null;
        int len = s.length();
        if (start < 0) start = 0;
        if (end > len) end = len;
        if (start > end) return "";
        return s.substring(start, end);
    }

    public static String insert(String s, int index, String toInsert) {
        if (s == null) s = "";
        if (toInsert == null) toInsert = "";
        if (index < 0) index = 0;
        if (index > s.length()) index = s.length();
        return s.substring(0, index) + toInsert + s.substring(index);
    }

    public static String remove(String s, String target) {
        if (s == null || target == null || target.isEmpty()) return s;
        return s.replace(target, "");
    }

    public static String replace(String s, String target, String replacement) {
        if (s == null || target == null) return s;
        if (replacement == null) replacement = "";
        return s.replace(target, replacement);
    }

    public static String trimAndCollapseSpaces(String s) {
        if (s == null) return null;
        return s.trim().replaceAll("\\s+", " ");
    }

    public static String repeat(String s, int times) {
        if (s == null) return null;
        if (times <= 0) return "";
        StringBuilder sb = new StringBuilder(s.length() * times);
        for (int i = 0; i < times; i++) {
            sb.append(s);
        }
        return sb.toString();
    }

    public static boolean isPalindrome(String s) {
        if (s == null) return false;
        String cleaned = s.replaceAll("[^A-Za-z0-9]", "").toLowerCase();
        return new StringBuilder(cleaned).reverse().toString().equals(cleaned);
    }

    public static int countOccurrences(String s, String target) {
        if (s == null || target == null || target.isEmpty()) return 0;
        int count = 0;
        int idx = 0;
        while ((idx = s.indexOf(target, idx)) != -1) {
            count++;
            idx += target.length();
        }
        return count;
    }

    public static String caesarShift(String s, int shift) {
        if (s == null) return null;
        StringBuilder sb = new StringBuilder(s.length());
        int k = ((shift % 26) + 26) % 26;
        for (char c : s.toCharArray()) {
            if (c >= 'a' && c <= 'z') {
                sb.append((char) ('a' + (c - 'a' + k) % 26));
            } else if (c >= 'A' && c <= 'Z') {
                sb.append((char) ('A' + (c - 'A' + k) % 26));
            } else {
                sb.append(c);
            }
        }
        return sb.toString();
    }

    public static String[] split(String s, String delimiter) {
        if (s == null) return new String[0];
        if (delimiter == null || delimiter.isEmpty()) {
            String[] chars = new String[s.length()];
            for (int i = 0; i < s.length(); i++) {
                chars[i] = String.valueOf(s.charAt(i));
            }
            return chars;
        }
        return s.split(java.util.regex.Pattern.quote(delimiter), -1);
    }

    public static void main(String[] args) {
        String original = "  hello   world  ";
        String copied = copy(original);
        System.out.println("Original: '" + original + "'");
        System.out.println("Copied: '" + copied + "'");
        System.out.println("Concatenated: " + concatenate("Hello, ", "World!"));
        System.out.println("Joined: " + concatenateAll(", ", "a", "b", "c"));
        System.out.println("Reversed: " + reverse("abcdef"));
        System.out.println("Uppercase: " + toUpperCase("java"));
        System.out.println("Lowercase: " + toLowerCase("JAVA"));
        System.out.println("Capitalized: " + capitalize("hELLO"));
        System.out.println("Substring(0,5): " + substring("programming", 0, 5));
        System.out.println("Insert: " + insert("Hello World", 6, "Java "));
        System.out.println("Remove 'l': " + remove("hello", "l"));
        System.out.println("Replace: " + replace("foo bar foo", "foo", "baz"));
        System.out.println("Trim & collapse: '" + trimAndCollapseSpaces(original) + "'");
        System.out.println("Repeat: " + repeat("ab", 3));
        System.out.println("Palindrome 'A man, a plan, a canal: Panama': "
                + isPalindrome("A man, a plan, a canal: Panama"));
        System.out.println("Occurrences of 'ana' in 'banana': "
                + countOccurrences("banana", "ana"));
        System.out.println("Caesar shift 3: " + caesarShift("Hello, World!", 3));
        System.out.println("Split by ',': " + String.join("|", split("a,b,,c", ",")));
    }
}