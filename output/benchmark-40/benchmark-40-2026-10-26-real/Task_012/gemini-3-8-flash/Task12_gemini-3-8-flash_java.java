public class Solution {
    public static String high(String s) {
        String[] words = s.split(" ");
        String highestWord = "";
        int maxScore = -1;

        for (String word : words) {
            int currentScore = 0;
            for (int i = 0; i < word.length(); i++) {
                currentScore += word.charAt(i) - 'a' + 1;
            }

            if (currentScore > maxScore) {
                maxScore = currentScore;
                highestWord = word;
            }
        }

        return highestWord;
    }
}