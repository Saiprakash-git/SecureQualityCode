public class Kata {
    public static boolean validatePin(String pin) {
        if (pin == null || (pin.length() != 4 && pin.length() != 6)) {
            return false;
        }

        for (int i = 0; i < pin.length(); i++) {
            if (pin.charAt(i) < '0' || pin.charAt(i) > '9') {
                return false;
            }
        }

        return true;
    }
}