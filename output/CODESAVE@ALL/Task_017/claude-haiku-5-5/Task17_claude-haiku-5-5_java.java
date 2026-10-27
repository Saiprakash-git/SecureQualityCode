import java.util.Scanner;

public class JadenCase {

    public static String toJadenCase(String phrase) {
        if (phrase == null || phrase.isEmpty()) {
            return null;
        }
        String[] words = phrase.split(" ", -1);
        StringBuilder result = new StringBuilder();
        for (int i = 0; i < words.length; i++) {
            String w = words[i];
            if (!w.isEmpty()) {
                result.append(Character.toUpperCase(w.charAt(0)));
                result.append(w.substring(1));
            }
            if (i < words.length - 1) {
                result.append(' ');
            }
        }
        return result.toString();
    }

    public static void main(String[] args) {
        String sample = "How can mirrors be real if our eyes aren't real";
        System.out.println(toJadenCase(sample));
    }
}