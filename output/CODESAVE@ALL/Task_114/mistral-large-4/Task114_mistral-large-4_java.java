public class StringOperations {

    public static String copyString(String source) {
        return new String(source);
    }

    public static String concatenateStrings(String str1, String str2) {
        return str1.concat(str2);
    }

    public static String concatenateMultiple(String... strings) {
        StringBuilder sb = new StringBuilder();
        for (String s : strings) {
            sb.append(s);
        }
        return sb.toString();
    }

    public static String reverseString(String input) {
        return new StringBuilder(input).reverse().toString();
    }

    public static boolean isPalindrome(String input) {
        String cleaned = input.replaceAll("[^a-zA-Z0-9]", "").toLowerCase();
        String reversed = reverseString(cleaned);
        return cleaned.equals(reversed);
    }

    public static int countVowels(String input) {
        int count = 0;
        String vowels = "aeiouAEIOU";
        for (char c : input.toCharArray()) {
            if (vowels.indexOf(c) != -1) {
                count++;
            }
        }
        return count;
    }

    public static int countConsonants(String input) {
        int count = 0;
        String consonants = "bcdfghjklmnpqrstvwxyzBCDFGHJKLMNPQRSTVWXYZ";
        for (char c : input.toCharArray()) {
            if (consonants.indexOf(c) != -1) {
                count++;
            }
        }
        return count;
    }

    public static String replaceCharacter(String input, char oldChar, char newChar) {
        return input.replace(oldChar, newChar);
    }

    public static String[] splitString(String input, String delimiter) {
        return input.split(delimiter);
    }

    public static String removeWhitespace(String input) {
        return input.replaceAll("\\s+", "");
    }

    public static String toUpperCase(String input) {
        return input.toUpperCase();
    }

    public static String toLowerCase(String input) {
        return input.toLowerCase();
    }

    public static boolean containsSubstring(String input, String substring) {
        return input.contains(substring);
    }

    public static String removeDuplicates(String input) {
        StringBuilder result = new StringBuilder();
        for (char c : input.toCharArray()) {
            if (result.indexOf(String.valueOf(c)) == -1) {
                result.append(c);
            }
        }
        return result.toString();
    }

    public static String capitalizeWords(String input) {
        String[] words = input.split("\\s+");
        StringBuilder result = new StringBuilder();
        for (String word : words) {
            if (word.length() > 0) {
                result.append(Character.toUpperCase(word.charAt(0)));
                if (word.length() > 1) {
                    result.append(word.substring(1).toLowerCase());
                }
                result.append(" ");
            }
        }
        return result.toString().trim();
    }

    public static int countWords(String input) {
        if (input == null || input.trim().isEmpty()) {
            return 0;
        }
        return input.trim().split("\\s+").length;
    }

    public static String repeatString(String input, int times) {
        return input.repeat(times);
    }

    public static String trimString(String input) {
        return input.trim();
    }

    public static String getSubstring(String input, int start, int end) {
        return input.substring(start, end);
    }

    public static int stringLength(String input) {
        return input.length();
    }

    public static boolean stringsEqual(String str1, String str2) {
        return str1.equals(str2);
    }

    public static boolean stringsEqualIgnoreCase(String str1, String str2) {
        return str1.equalsIgnoreCase(str2);
    }

    public static String insertString(String original, String toInsert, int position) {
        return original.substring(0, position) + toInsert + original.substring(position);
    }

    public static String removeCharacter(String input, char charToRemove) {
        return input.replace(String.valueOf(charToRemove), "");
    }

    public static void main(String[] args) {
        String str1 = "Hello";
        String str2 = "World";

        System.out.println("Original strings: " + str1 + ", " + str2);
        System.out.println("Copy: " + copyString(str1));
        System.out.println("Concatenation: " + concatenateStrings(str1, str2));
        System.out.println("Reverse: " + reverseString(str1));
        System.out.println("Is Palindrome: " + isPalindrome("racecar"));
        System.out.println("Vowel count: " + countVowels(str1));
        System.out.println("Consonant count: " + countConsonants(str1));
        System.out.println("Replace 'l' with 'L': " + replaceCharacter(str1, 'l', 'L'));
        System.out.println("Remove whitespace: " + removeWhitespace("Hello World Test"));
        System.out.println("Upper case: " + toUpperCase(str1));
        System.out.println("Lower case: " + toLowerCase(str2));
        System.out.println("Contains 'ell': " + containsSubstring(str1, "ell"));
        System.out.println("Remove duplicates: " + removeDuplicates("programming"));
        System.out.println("Capitalize words: " + capitalizeWords("hello world java"));
        System.out.println("Word count: " + countWords("Hello World Java Programming"));
        System.out.println("Repeat: " + repeatString("Hi ", 3));
        System.out.println("Length: " + stringLength(str1));
    }
}