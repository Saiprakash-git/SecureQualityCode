package claude.task15;
public class Tower {
    public static String[] towerBuilder(int nFloors) {
        String[] tower = new String[nFloors];
        int width = 2 * nFloors - 1;

        for (int i = 0; i < nFloors; i++) {
            int stars = 2 * i + 1;
            int spaces = (width - stars) / 2;

            StringBuilder sb = new StringBuilder();

            for (int j = 0; j < spaces; j++) {
                sb.append(' ');
            }

            for (int j = 0; j < stars; j++) {
                sb.append('*');
            }

            for (int j = 0; j < spaces; j++) {
                sb.append(' ');
            }

            tower[i] = sb.toString();
        }

        return tower;
    }
}