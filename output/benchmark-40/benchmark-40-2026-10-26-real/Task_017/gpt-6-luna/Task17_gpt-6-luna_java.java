public class JadenCase {
    public String toJadenCase(String phrase) {
        if (phrase == null || phrase.isEmpty()) {
            return null;
        }

        StringBuilder result = new StringBuilder(phrase);
        boolean atWordStart = true;

        for (int i = 0; i < result.length(); i++) {
            char c = result.charAt(i);
            if (Character.isWhitespace(c)) {
                atWordStart = true;
            } else if (atWordStart) {
                result.setCharAt(i, Character.toUpperCase(c));
                atWordStart = false;
            }
        }

        return result.toString();
    }
}