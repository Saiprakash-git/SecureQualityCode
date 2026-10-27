import java.util.Scanner;

public class Main {
    public static String copy(String text) {
        return new String(text);
    }

    public static String concatenate(String first, String second) {
        return first + second;
    }

    public static String reverse(String text) {
        return new StringBuilder(text).reverse().toString();
    }

    public static String toUpperCase(String text) {
        return text.toUpperCase();
    }

    public static String toLowerCase(String text) {
        return text.toLowerCase();
    }

    public static String replace(String text, String target, String replacement) {
        return text.replace(target, replacement);
    }

    public static String substring(String text, int start, int end) {
        return text.substring(start, end);
    }

    public static int countOccurrences(String text, String target) {
        if (target.isEmpty()) {
            return text.length() + 1;
        }

        int count = 0;
        int index = 0;
        while ((index = text.indexOf(target, index)) != -1) {
            count++;
            index += target.length();
        }
        return count;
    }

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);

        while (true) {
            System.out.println("\nString Operations");
            System.out.println("1. Copy");
            System.out.println("2. Concatenate");
            System.out.println("3. Reverse");
            System.out.println("4. Convert to uppercase");
            System.out.println("5. Convert to lowercase");
            System.out.println("6. Replace text");
            System.out.println("7. Extract substring");
            System.out.println("8. Count occurrences");
            System.out.println("0. Exit");
            System.out.print("Choose an operation: ");

            String choice = scanner.nextLine().trim();

            try {
                switch (choice) {
                    case "1":
                        System.out.print("Enter text: ");
                        System.out.println("Result: " + copy(scanner.nextLine()));
                        break;
                    case "2":
                        System.out.print("Enter the first string: ");
                        String first = scanner.nextLine();
                        System.out.print("Enter the second string: ");
                        String second = scanner.nextLine();
                        System.out.println("Result: " + concatenate(first, second));
                        break;
                    case "3":
                        System.out.print("Enter text: ");
                        System.out.println("Result: " + reverse(scanner.nextLine()));
                        break;
                    case "4":
                        System.out.print("Enter text: ");
                        System.out.println("Result: " + toUpperCase(scanner.nextLine()));
                        break;
                    case "5":
                        System.out.print("Enter text: ");
                        System.out.println("Result: " + toLowerCase(scanner.nextLine()));
                        break;
                    case "6":
                        System.out.print("Enter text: ");
                        String textToReplace = scanner.nextLine();
                        System.out.print("Text to find: ");
                        String target = scanner.nextLine();
                        System.out.print("Replacement text: ");
                        String replacement = scanner.nextLine();
                        System.out.println("Result: " + replace(textToReplace, target, replacement));
                        break;
                    case "7":
                        System.out.print("Enter text: ");
                        String text = scanner.nextLine();
                        System.out.print("Start index: ");
                        int start = Integer.parseInt(scanner.nextLine().trim());
                        System.out.print("End index: ");
                        int end = Integer.parseInt(scanner.nextLine().trim());
                        System.out.println("Result: " + substring(text, start, end));
                        break;
                    case "8":
                        System.out.print("Enter text: ");
                        String textToSearch = scanner.nextLine();
                        System.out.print("Text to count: ");
                        String searchTarget = scanner.nextLine();
                        System.out.println("Occurrences: " + countOccurrences(textToSearch, searchTarget));
                        break;
                    case "0":
                        scanner.close();
                        return;
                    default:
                        System.out.println("Invalid choice. Please try again.");
                }
            } catch (IndexOutOfBoundsException | NumberFormatException exception) {
                System.out.println("Invalid input or index.");
            }
        }
    }
}