public class HighestScoringWord {
    public static String find(String input) {
        String[] words = input.split(" ");
        String bestWord = words[0];
        int bestScore = score(words[0]);

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
        int sum = 0;
        for (char c : word.toCharArray()) {
            sum += c - 'a' + 1;
        }
        return sum;
    }
}