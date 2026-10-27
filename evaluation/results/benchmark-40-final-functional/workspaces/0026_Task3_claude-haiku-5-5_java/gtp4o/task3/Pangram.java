package gtp4o.task3;
public class Pangram {

    public static boolean isPangram(String input) {
        if (input == null) {
            return false;
        }
        boolean[] seen = new boolean[26];
        int count = 0;
        for (char c : input.toCharArray()) {
            char lower = Character.toLowerCase(c);
            if (lower >= 'a' && lower <= 'z') {
                int idx = lower - 'a';
                if (!seen[idx]) {
                    seen[idx] = true;
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
        String sample = args.length > 0 ? String.join(" ", args) : "The quick brown fox jumps over the lazy dog";
        System.out.println(isPangram(sample) ? "True" : "False");
    }
}