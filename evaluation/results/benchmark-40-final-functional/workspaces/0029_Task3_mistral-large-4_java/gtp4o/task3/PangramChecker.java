package gtp4o.task3;
public class PangramChecker {
    public static boolean isPangram(String text) {
        if (text == null) {
            return false;
        }

        boolean[] seen = new boolean[26];
        int count = 0;

        for (int i = 0; i < text.length(); i++) {
            char c = text.charAt(i);

            if (c >= 'A' && c <= 'Z') {
                c = (char) (c - 'A' + 'a');
            }

            if (c >= 'a' && c <= 'z') {
                int index = c - 'a';
                if (!seen[index]) {
                    seen[index] = true;
                    count++;
                    if (count == 26) {
                        return true;
                    }
                }
            }
        }

        return count == 26;
    }

    public static void main(String[] args) {
        System.out.println(isPangram("The quick brown fox jumps over the lazy dog"));
        System.out.println(isPangram("Hello World"));
    }
}