public class TowerBuilder {
    public static String[] buildTower(int floors) {
        String[] tower = new String[floors];
        for (int i = 0; i < floors; i++) {
            int spaces = floors - i - 1;
            int stars = 2 * i + 1;
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