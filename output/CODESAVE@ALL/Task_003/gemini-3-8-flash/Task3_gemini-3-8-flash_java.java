public class PangramChecker {

    public static boolean isPangram(String sentence) {
        if (sentence == null) {
            return false;
        }

        int mask = 0;
        int targetMask = (1 << 26) - 1;

        for (int i = 0; i < sentence.length(); i++) {
            char c = sentence.charAt(i);
            if (c >= 'a' && c <= 'z') {
                mask |= (1 << (c - 'a'));
            } else if (c >= 'A' && c <= 'Z') {
                mask |= (1 << (c - 'A'));
            }

            if (mask == targetMask) {
                return true;
            }
        }

        return mask == targetMask;
    }

    public static void main(String[] args) {
        String test1 = "The quick brown fox jumps over the lazy dog";
        String test2 = "You shall not pass!";

        System.out.println(isPangram(test1)); // true
        System.out.println(isPangram(test2)); // false
    }
}