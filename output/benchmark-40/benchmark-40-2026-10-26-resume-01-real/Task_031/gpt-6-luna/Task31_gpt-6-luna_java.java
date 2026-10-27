public class DnaStrand {
    public static String makeComplement(String dna) {
        StringBuilder complement = new StringBuilder(dna.length());
        for (int i = 0; i < dna.length(); i++) {
            switch (dna.charAt(i)) {
                case 'A':
                    complement.append('T');
                    break;
                case 'T':
                    complement.append('A');
                    break;
                case 'C':
                    complement.append('G');
                    break;
                case 'G':
                    complement.append('C');
                    break;
                default:
                    complement.append(dna.charAt(i));
            }
        }
        return complement.toString();
    }
}