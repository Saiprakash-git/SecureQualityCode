import java.io.InputStream;
import java.io.IOException;
import java.util.ArrayList;

public class Main {
    private static class FastScanner {
        private final InputStream in;
        private final byte[] buffer = new byte[1 << 16];
        private int ptr = 0;
        private int len = 0;

        FastScanner(InputStream in) {
            this.in = in;
        }

        private int read() throws IOException {
            if (ptr >= len) {
                len = in.read(buffer);
                ptr = 0;
                if (len <= 0) {
                    return -1;
                }
            }
            return buffer[ptr++];
        }

        Integer nextInt() throws IOException {
            int c;
            do {
                c = read();
                if (c == -1) {
                    return null;
                }
            } while (c <= ' ');

            int sign = 1;
            if (c == '-') {
                sign = -1;
                c = read();
            }

            int value = 0;
            while (c != -1 && c > ' ') {
                value = value * 10 + (c - '0');
                c = read();
            }

            return value * sign;
        }
    }

    public static void main(String[] args) throws Exception {
        FastScanner fs = new FastScanner(System.in);
        ArrayList<Integer> input = new ArrayList<>();

        Integer value;
        while ((value = fs.nextInt()) != null) {
            input.add(value);
        }

        if (input.isEmpty()) {
            return;
        }

        int[] array;
        int first = input.get(0);

        if (input.size() > 1 && first >= 0 && first == input.size() - 1) {
            array = new int[first];
            for (int i = 0; i < first; i++) {
                array[i] = input.get(i + 1);
            }
        } else {
            array = new int[input.size()];
            for (int i = 0; i < input.size(); i++) {
                array[i] = input.get(i);
            }
        }

        if (array.length == 0) {
            System.out.println("Maximum sum: 0");
            System.out.println("Subarray:");
            return;
        }

        int maxEndingHere = array[0];
        int maxSum = array[0];
        int start = 0;
        int end = 0;
        int tempStart = 0;

        for (int i = 1; i < array.length; i++) {
            if (array[i] > maxEndingHere + array[i]) {
                maxEndingHere = array[i];
                tempStart = i;
            } else {
                maxEndingHere += array[i];
            }

            if (maxEndingHere > maxSum) {
                maxSum = maxEndingHere;
                start = tempStart;
                end = i;
            }
        }

        StringBuilder output = new StringBuilder();
        output.append("Maximum sum: ").append(maxSum).append(System.lineSeparator());
        output.append("Subarray:");

        for (int i = start; i <= end; i++) {
            output.append(' ').append(array[i]);
        }

        System.out.print(output);
    }
}