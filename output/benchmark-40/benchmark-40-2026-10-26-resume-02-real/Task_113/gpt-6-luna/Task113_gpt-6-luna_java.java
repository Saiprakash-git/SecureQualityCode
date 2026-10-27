import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.List;

public class Main {
    public static String concatenate(String... strings) {
        StringBuilder result = new StringBuilder();
        for (String value : strings) {
            result.append(value);
        }
        return result.toString();
    }

    public static void main(String[] args) throws Exception {
        BufferedReader reader = new BufferedReader(new InputStreamReader(System.in));
        List<String> strings = new ArrayList<>();
        String line;

        while ((line = reader.readLine()) != null) {
            strings.add(line);
        }

        System.out.print(concatenate(strings.toArray(new String[0])));
    }
}