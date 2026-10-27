package claude.task15;
public class Main {
    public static String[] buildTower(int nFloors) {
        String[] tower = new String[nFloors];
        int width = 2 * nFloors - 1;
        for (int i = 0; i < nFloors; i++) {
            int stars = 2 * i + 1;
            int spaces = (width - stars) / 2;
            StringBuilder sb = new StringBuilder();
            for (int s = 0; s < spaces; s++) sb.append(' ');
            for (int s = 0; s < stars; s++) sb.append('*');
            for (int s = 0; s < spaces; s++) sb.append(' ');
            tower[i] = sb.toString();
        }
        return tower;
    }

    public static void main(String[] args) {
        int floors = args.length > 0 ? Integer.parseInt(args[0]) : 3;
        String[] tower = buildTower(floors);
        for (String line : tower) {
            System.out.println("\"" + line + "\"");
        }
    }
}