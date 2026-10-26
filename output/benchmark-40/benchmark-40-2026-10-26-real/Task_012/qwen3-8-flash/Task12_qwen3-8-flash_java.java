public class Solution {
    public static String high(String s) {
        String[] words = s.split(" ");
        String bestWord = words[0];
        int bestScore = score(bestWord);

        for (int i = 1; i < words.length; i++) {
            int currentScore = score(words[i]);
            if (currentScore > bestScore) {
                bestScore = currentScore;
                bestWord = words[i];
            }
        }

        return bestWord;
    }

    private static int score(String word) {
        int total = 0;
        for (int i = 0; i < word.length(); i++) {
            total += word.charAt(i) - 'a' + 1;
        }
        return total;
    }
}