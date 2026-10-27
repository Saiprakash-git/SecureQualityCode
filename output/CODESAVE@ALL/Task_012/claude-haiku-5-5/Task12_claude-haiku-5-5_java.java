public class Solution {
    public static String high(String s) {
        String[] words = s.trim().split("\\s+");
        String best = words[0];
        int bestScore = score(words[0]);
        for (int i = 1; i < words.length; i++) {
            int sc = score(words[i]);
            if (sc > bestScore) {
                bestScore = sc;
                best = words[i];
            }
        }
        return best;
    }

    private static int score(String w) {
        int total = 0;
        for (char c : w.toCharArray()) {
            if (c >= 'a' && c <= 'z') {
                total += c - 'a' + 1;
            }
        }
        return total;
    }
}