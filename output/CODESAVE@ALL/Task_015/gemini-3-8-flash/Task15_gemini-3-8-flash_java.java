public class TowerBuilder {
    public static String[] buildTower(int nFloors) {
        String[] tower = new String[nFloors];
        int width = 2 * nFloors - 1;

        for (int i = 0; i < nFloors; i++) {
            int stars = 2 * i + 1;
            int spaces = nFloors - 1 - i;
            StringBuilder floor = new StringBuilder(width);

            for (int j = 0; j < spaces; j++) {
                floor.append(' ');
            }
            for (int j = 0; j < stars; j++) {
                floor.append('*');
            }
            for (int j = 0; j < spaces; j++) {
                floor.append(' ');
            }

            tower[i] = floor.toString();
        }

        return tower;
    }
}