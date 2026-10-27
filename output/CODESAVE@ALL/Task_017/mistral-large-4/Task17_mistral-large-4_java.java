public class JadenCase {
    public String toJadenCase(String phrase) {
        if (phrase == null || phrase.isEmpty()) {
            return null;
        }
        char[] chars = phrase.toCharArray();
        boolean capitalizeNext = true;
        for (int i = 0; i < chars.length; i++) {
            if (chars[i] == ' ') {
                capitalizeNext = true;
            } else if (capitalizeNext) {
                chars[i] = Character.toUpperCase(chars[i]);
                capitalizeNext = false;
            }
        }
        return new String(chars);
    }
}