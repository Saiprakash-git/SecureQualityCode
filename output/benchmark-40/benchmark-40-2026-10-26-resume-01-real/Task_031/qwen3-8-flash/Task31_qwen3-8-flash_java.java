public class Solution {
    public static String dnaStrand(String d) {
        if (d == null) {
            return "";
        }

        StringBuilder result = new StringBuilder(d.length());

        for (int i = 0; i < d.length(); i++) {
            char c = d.charAt(i);

            switch (c) {
                case 'A':
                    result.append('T');
                    break;
                case 'T':
                    result.append('A');
                    break;
                case 'C':
                    result.append('G');
                    break;
                case 'G':
                    result.append('C');
                    break;
                default:
                    result.append(c);
                    break;
            }
        }

        return result.toString();
    }
}