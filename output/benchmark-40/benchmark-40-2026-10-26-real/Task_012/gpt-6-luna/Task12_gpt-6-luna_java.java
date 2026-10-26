public class Kata {
    public static String high(String s) {
        String bestWord = "";
        int bestScore = -1;
        int wordStart = 0;

        for (int i = 0; i <= s.length(); i++) {
            if (i == s.length() || s.charAt(i) == ' ') {
                int score = 0;
                for (int j = wordStart; j < i; j++) {
                    score += s.charAt(j) - 'a' + 1;
                }
                if (score > bestScore) {
                    bestScore = score;
                    bestWord = s.substring(wordStart, i);
                }
                wordStart = i + 1;
            }
        }

        return bestWord;
    }
}