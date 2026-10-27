public class Solution {
    public static boolean validatePin(String pin) {
        if (pin == null) {
            return false;
        }
        int len = pin.length();
        if (len != 4 && len != 6) {
            return false;
        }
        for (int i = 0; i < len; i++) {
            char c = pin.charAt(i);
            if (c < '0' || c > '9') {
                return false;
            }
        }
        return true;
    }
}