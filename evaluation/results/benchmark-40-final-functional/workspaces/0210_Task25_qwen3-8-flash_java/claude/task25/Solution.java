package claude.task25;
import java.util.ArrayList;
import java.util.List;

public class Solution {
    public static List<String> number(List<String> lines) {
        List<String> result = new ArrayList<>();
        int number = 1;

        for (String line : lines) {
            result.add(number + ": " + line);
            number++;
        }

        return result;
    }
}