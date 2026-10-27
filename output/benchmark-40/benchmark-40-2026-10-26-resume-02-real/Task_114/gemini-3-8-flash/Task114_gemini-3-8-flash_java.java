public class StringOperations {

    public static String copy(String input) {
        if (input == null) {
            return null;
        }
        return new String(input.toCharArray());
    }

    public static String concatenate(String... strings) {
        if (strings == null) {
            return "";
        }
        StringBuilder sb = new StringBuilder();
        for (String str : strings) {
            if (str != null) {
                sb.append(str);
            }
        }
        return sb.toString();
    }

    public static String reverse(String input) {
        if (input == null) {
            return null;
        }
        return new StringBuilder(input).reverse().toString();
    }

    public static String toTitleCase(String input) {
        if (input == null || input.isEmpty()) {
            return input;
        }
        StringBuilder result = new StringBuilder();
        boolean capitalizeNext = true;
        for (char c : input.toCharArray()) {
            if (Character.isWhitespace(c)) {
                capitalizeNext = true;
                result.append(c);
            } else if (capitalizeNext) {
                result.append(Character.toTitleCase(c));
                capitalizeNext = false;
            } else {
                result.append(Character.toLowerCase(c));
            }
        }
        return result.toString();
    }

    public static boolean isPalindrome(String input) {
        if (input == null) {
            return false;
        }
        String clean = input.replaceAll("[^a-zA-Z0-9]", "").toLowerCase();
        int left = 0;
        int right = clean.length() - 1;
        while (left < right) {
            if (clean.charAt(left) != clean.charAt(right)) {
                return false;
            }
            left++;
            right--;
        }
        return true;
    }

    public static int countOccurrences(String text, String target) {
        if (text == null || target == null || target.isEmpty()) {
            return 0;
        }
        int count = 0;
        int index = 0;
        while ((index = text.indexOf(target, index)) != -1) {
            count++;
            index += target.length();
        }
        return count;
    }

    public static String removeWhitespace(String input) {
        if (input == null) {
            return null;
        }
        return input.replaceAll("\\s+", "");
    }

    public static String padLeft(String input, int length, char padChar) {
        if (input == null) {
            input = "";
        }
        if (input.length() >= length) {
            return input;
        }
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < length - input.length(); i++) {
            sb.append(padChar);
        }
        sb.append(input);
        return sb.toString();
    }

    public static String padRight(String input, int length, char padChar) {
        if (input == null) {
            input = "";
        }
        if (input.length() >= length) {
            return input;
        }
        StringBuilder sb = new StringBuilder(input);
        for (int i = 0; i < length - input.length(); i++) {
            sb.append(padChar);
        }
        return sb.toString();
    }

    public static String truncate(String input, int maxLength) {
        if (input == null) {
            return null;
        }
        if (input.length() <= maxLength) {
            return input;
        }
        if (maxLength <= 3) {
            return input.substring(0, maxLength);
        }
        return input.substring(0, maxLength - 3) + "...";
    }

    public static String replaceSubstring(String source, String target, String replacement) {
        if (source == null || target == null || replacement == null) {
            return source;
        }
        return source.replace(target, replacement);
    }

    public static void main(String[] args) {
        String original = "Hello World";
        String copied = copy(original);
        System.out.println("Original: " + original);
        System.out.println("Copied: " + copied);

        String concatenated = concatenate("Java", " ", "String", " ", "Operations");
        System.out.println("Concatenated: " + concatenated);

        String reversed = reverse(original);
        System.out.println("Reversed: " + reversed);

        String titleCase = toTitleCase("the quick brown fox jumps over the lazy dog");
        System.out.println("Title Case: " + titleCase);

        String palindromeTest = "A man, a plan, a canal: Panama";
        System.out.println("Is Palindrome ('" + palindromeTest + "'): " + isPalindrome(palindromeTest));

        int occurrences = countOccurrences("banana bandana banana", "an");
        System.out.println("Occurrences of 'an': " + occurrences);

        String stripped = removeWhitespace("  Remove   all \t whitespace \n here  ");
        System.out.println("Stripped: " + stripped);

        String paddedLeft = padLeft("42", 6, '0');
        System.out.println("Padded Left: " + paddedLeft);

        String paddedRight = padRight("Item", 10, '.');
        System.out.println("Padded Right: " + paddedRight);

        String truncated = truncate("This is a long sentence that needs truncating.", 20);
        System.out.println("Truncated: " + truncated);

        String replaced = replaceSubstring("The quick brown fox", "fox", "bear");
        System.out.println("Replaced: " + replaced);
    }
}