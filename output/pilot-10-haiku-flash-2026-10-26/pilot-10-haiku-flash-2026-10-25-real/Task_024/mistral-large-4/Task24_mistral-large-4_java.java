public class PinValidator {
    public static boolean validatePin(String pin) {
        return pin != null && pin.matches("[0-9]{4}|[0-9]{6}");
    }
}