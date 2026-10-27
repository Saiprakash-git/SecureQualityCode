import java.util.*;

public class DnaComplement {

    public static String makeComplement(String dna) {
        StringBuilder sb = new StringBuilder(dna.length());
        for (char c : dna.toCharArray()) {
            switch (c) {
                case 'A': sb.append('T'); break;
                case 'T': sb.append('A'); break;
                case 'C': sb.append('G'); break;
                case 'G': sb.append('C'); break;
                default: sb.append(c); break;
            }
        }
        return sb.toString();
    }

    public static void main(String[] args) {
        Scanner scanner = new Scanner(System.in);
        if (scanner.hasNextLine()) {
            String dna = scanner.nextLine().trim();
            System.out.println(makeComplement(dna));
        }
        scanner.close();
    }
}