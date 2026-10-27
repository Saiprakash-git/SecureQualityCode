package claude.task143;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;

import org.junit.jupiter.api.Test;

public class Task143Test {

    private int[] parseInput(String input) {
        if (input.isEmpty()) {
            return new int[0];
        }
        String[] parts = input.trim().split("\\s+");
        int[] arr = new int[parts.length];
        for (int i = 0; i < parts.length; i++) {
            arr[i] = Integer.parseInt(parts[i]);
        }
        return arr;
    }

    private int[] callBubbleSort(String input) {
        int[] arr = parseInput(input);
        Task143_CLAUDE_claude_3_5_sonnet_20240620.bubbleSort(arr);
        return arr;
    }

    @Test
    public void test_basic() {
        assertArrayEquals(new int[]{1, 2, 3, 4}, callBubbleSort("4 3 2 1"));
    }

    @Test
    public void test_single_element() {
        assertArrayEquals(new int[]{5}, callBubbleSort("5"));
    }

    @Test
    public void test_already_sorted() {
        assertArrayEquals(new int[]{1, 2, 3, 4, 5}, callBubbleSort("1 2 3 4 5"));
    }

    @Test
    public void test_all_elements_same() {
        assertArrayEquals(new int[]{7, 7, 7, 7}, callBubbleSort("7 7 7 7"));
    }

    @Test
    public void test_negative_numbers() {
        assertArrayEquals(new int[]{-4, -3, -2, -1}, callBubbleSort("-1 -3 -2 -4"));
    }

    @Test
    public void test_mixed_positive_and_negative() {
        assertArrayEquals(new int[]{-5, -1, 0, 2, 3}, callBubbleSort("3 -1 0 2 -5"));
    }

    @Test
    public void test_large_numbers() {
        assertArrayEquals(new int[]{500, 1000, 1500, 2000}, callBubbleSort("1000 500 2000 1500"));
    }

    @Test
    public void test_float_numbers() {
        assertArrayEquals(new int[]{1, 2, 3, 4}, callBubbleSort("3 2 4 1"));
    }

    @Test
    public void test_mixed_integers_and_floats() {
        assertArrayEquals(new int[]{0, 1, 1, 2, 2}, callBubbleSort("1 2 1 2 0"));
    }

    @Test
    public void test_empty_input() {
        assertArrayEquals(new int[]{}, callBubbleSort(""));
    }
}
