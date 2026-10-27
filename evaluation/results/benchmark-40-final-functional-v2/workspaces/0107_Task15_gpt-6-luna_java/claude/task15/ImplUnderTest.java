package claude.task15;
class Kata {
    public static String[] towerBuilder(int nFloors) {
        String[] tower = new String[nFloors];
        int width = 2 * nFloors - 1;

        for (int floor = 0; floor < nFloors; floor++) {
            int stars = 2 * floor + 1;
            int spaces = (width - stars) / 2;
            StringBuilder row = new StringBuilder(width);

            for (int i = 0; i < spaces; i++) {
                row.append(' ');
            }
            for (int i = 0; i < stars; i++) {
                row.append('*');
            }
            for (int i = 0; i < spaces; i++) {
                row.append(' ');
            }

            tower[floor] = row.toString();
        }

        return tower;
    }
}