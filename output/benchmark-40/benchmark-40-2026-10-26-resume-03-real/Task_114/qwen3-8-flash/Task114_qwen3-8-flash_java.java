import java.util.Arrays;

public class Main {

    public static String copyString(String source) {
        if (source == null) {
            return null;
        }
        return new String(source);
    }

    public static String concatenate(String first, String second) {
        if (first == null) {
            first = "";
        }
        if (second == null) {
            second = "";
        }
        return first + second;
    }

    public static String reverseString(String source) {
        if (source == null) {
            return null;
        }
        return new StringBuilder(source).reverse().toString();
    }

    public static String toUpperCase(String source) {
        if (source == null) {
            return null;
        }
        return source.toUpperCase();
    }

    public static String toLowerCase(String source) {
        if (source == null) {
            return null;
        }
        return source.toLowerCase();
    }

    public static String trimWhitespace(String source) {
        if (source == null) {
            return null;
        }
        return source.trim();
    }

    public static String replaceText(String source, String target, String replacement) {
        if (source == null || target == null || replacement == null) {
            return source;
        }
        return source.replace(target, replacement);
    }

    public static String extractSubstring(String source, int beginIndex, int endIndex) {
        if (source == null) {
            return null;
        }
        if (beginIndex < 0) {
            beginIndex = 0;
        }
        if (endIndex > source.length()) {
            endIndex = source.length();
        }
        if (beginIndex > endIndex) {
            return "";
        }
        return source.substring(beginIndex, endIndex);
    }

    public static int getStringLength(String source) {
        if (source == null) {
            return 0;
        }
        return source.length();
    }

    public static int compareStrings(String first, String second) {
        if (first == null && second == null) {
            return 0;
        }
        if (first == null) {
            return -1;
        }
        if (second == null) {
            return 1;
        }
        return first.compareTo(second);
    }

    public static boolean containsText(String source, String target) {
        if (source == null || target == null) {
            return false;
        }
        return source.contains(target);
    }

    public static boolean startsWithText(String source, String prefix) {
        if (source == null || prefix == null) {
            return false;
        }
        return source.startsWith(prefix);
    }

    public static boolean endsWithText(String source, String suffix) {
        if (source == null || suffix == null) {
            return false;
        }
        return source.endsWith(suffix);
    }

    public static String[] splitText(String source, String delimiter) {
        if (source == null) {
            return new String[0];
        }
        if (delimiter == null || delimiter.isEmpty()) {
            return new String[]{source};
        }
        return source.split(delimiter);
    }

    public static String joinText(String[] parts, String delimiter) {
        if (parts == null) {
            return "";
        }
        if (delimiter == null) {
            delimiter = "";
        }
        return String.join(delimiter, parts);
    }

    public static char getCharacterAt(String source, int index) {
        if (source == null || index < 0 || index >= source.length()) {
            return '\0';
        }
        return source.charAt(index);
    }

    public static int findIndexOfText(String source, String target) {
        if (source == null || target == null) {
            return -1;
        }
        return source.indexOf(target);
    }

    public static int findLastIndexOfText(String source, String target) {
        if (source == null || target == null) {
            return -1;
        }
        return source.lastIndexOf(target);
    }

    public static String removeWhitespace(String source) {
        if (source == null) {
            return null;
        }
        return source.replaceAll("\\s+", "");
    }

    public static String toTitleCase(String source) {
        if (source == null || source.isEmpty()) {
            return source;
        }

        StringBuilder result = new StringBuilder();
        boolean capitalizeNext = true;

        for (int i = 0; i < source.length(); i++) {
            char current = source.charAt(i);

            if (Character.isWhitespace(current)) {
                capitalizeNext = true;
                result.append(current);
            } else if (capitalizeNext) {
                result.append(Character.toUpperCase(current));
                capitalizeNext = false;
            } else {
                result.append(Character.toLowerCase(current));
            }
        }

        return result.toString();
    }

    public static String repeatString(String source, int times) {
        if (source == null || times <= 0) {
            return "";
        }

        StringBuilder result = new StringBuilder();
        for (int i = 0; i < times; i++) {
            result.append(source);
        }
        return result.toString();
    }

    public static String numberToString(int number) {
        return String.valueOf(number);
    }

    public static String doubleToString(double number) {
        return String.valueOf(number);
    }

    public static String booleanToString(boolean value) {
        return String.valueOf(value);
    }

    public static void main(String[] args) {
        String original = "Hello World";
        String additional = " Java";
        String sentence = "java is fun";
        String csv = "apple,banana,orange";

        System.out.println("Original String: " + original);
        System.out.println("Copied String: " + copyString(original));
        System.out.println("Concatenated String: " + concatenate(original, additional));
        System.out.println("Reversed String: " + reverseString(original));
        System.out.println("Upper Case: " + toUpperCase(original));
        System.out.println("Lower Case: " + toLowerCase(original));
        System.out.println("Trimmed: " + trimWhitespace("   padded text   "));
        System.out.println("Replaced: " + replaceText(original, "World", "Java"));
        System.out.println("Substring: " + extractSubstring(original, 6, 11));
        System.out.println("Length: " + getStringLength(original));
        System.out.println("Compare: " + compareStrings("abc", "abd"));
        System.out.println("Contains: " + containsText(original, "World"));
        System.out.println("Starts With: " + startsWithText(original, "Hello"));
        System.out.println("Ends With: " + endsWithText(original, "World"));
        System.out.println("Split: " + Arrays.toString(splitText(csv, ",")));
        System.out.println("Join: " + joinText(splitText(csv, ","), " | "));
        System.out.println("Character At Index 6: " + getCharacterAt(original, 6));
        System.out.println("Index Of: " + findIndexOfText(original, "World"));
        System.out.println("Last Index Of: " + findLastIndexOfText("banana", "a"));
        System.out.println("Removed Whitespace: " + removeWhitespace("  a b  c  "));
        System.out.println("Title Case: " + toTitleCase(sentence));
        System.out.println("Repeated: " + repeatString("ab", 3));
        System.out.println("Number to String: " + numberToString(123));
        System.out.println("Double to String: " + doubleToString(9.81));
        System.out.println("Boolean to String: " + booleanToString(true));
    }
}