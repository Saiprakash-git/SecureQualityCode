import java.util.ArrayList;
import java.util.List;

public class LineNumbering {

    public static List<String> number(List<String> lines) {
        List<String> result = new ArrayList<>();
        if (lines == null) {
            return result;
        }
        for (int i = 0; i < lines.size(); i++) {
            result.add((i + 1) + ": " + lines.get(i));
        }
        return result;
    }

    public static void main(String[] args) {
        System.out.println(number(new ArrayList<>()));
        List<String> sample = new ArrayList<>();
        sample.add("a");
        sample.add("b");
        sample.add("c");
        System.out.println(number(sample));
    }
}