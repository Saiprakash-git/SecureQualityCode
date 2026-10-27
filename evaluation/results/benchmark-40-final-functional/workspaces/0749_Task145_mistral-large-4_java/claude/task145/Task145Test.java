package claude.task145;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

class Task145Test {

    @Test
    void testPositiveNumbers() {
        int[] arr = {1, 2, 3, 4, 5};
        int expected = 15;
        int actual = Task145_CLAUDE_claude_3_5_sonnet_20240620.maxSubarraySum(arr);
        assertEquals(expected, actual);
    }

    @Test
    void testNegativeNumbers() {
        int[] arr = {-1, -2, -3, -4, -5};
        int expected = -1;
        int actual = Task145_CLAUDE_claude_3_5_sonnet_20240620.maxSubarraySum(arr);
        assertEquals(expected, actual);
    }

    @Test
    void testMixedNumbers() {
        int[] arr = {1, -2, 3, 4, -1, 2, 1, -5, 4};
        int expected = 9;
        int actual = Task145_CLAUDE_claude_3_5_sonnet_20240620.maxSubarraySum(arr);
        assertEquals(expected, actual);
    }

    @Test
    void testSingleElement() {
        int[] arr = {42};
        int expected = 42;
        int actual = Task145_CLAUDE_claude_3_5_sonnet_20240620.maxSubarraySum(arr);
        assertEquals(expected, actual);
    }

    @Test
    void testAllZeroes() {
        int[] arr = {0, 0, 0, 0, 0};
        int expected = 0;
        int actual = Task145_CLAUDE_claude_3_5_sonnet_20240620.maxSubarraySum(arr);
        assertEquals(expected, actual);
    }

    @Test
    void testLargeArray() {
        int[] arr = new int[1000];
        for (int i = 0; i < arr.length; i++) {
            arr[i] = 1;
        }
        int expected = 1000;
        int actual = Task145_CLAUDE_claude_3_5_sonnet_20240620.maxSubarraySum(arr);
        assertEquals(expected, actual);
    }

    @Test
    void testAlternatingSigns() {
        int[] arr = {3, -1, 4, -1, 2, 1, -5, 4};
        int expected = 8;
        int actual = Task145_CLAUDE_claude_3_5_sonnet_20240620.maxSubarraySum(arr);
        assertEquals(expected, actual);
    }

    @Test
    void testLargePositiveAndNegativeNumbers() {
        int[] arr = {10, -1, -1, 10, -1, 10, -1};
        int expected = 27;
        int actual = Task145_CLAUDE_claude_3_5_sonnet_20240620.maxSubarraySum(arr);
        assertEquals(expected, actual);
    }

    @Test
    void testSubarrayAtTheEnd() {
        int[] arr = {1, -2, -3, 4, 5};
        int expected = 9;
        int actual = Task145_CLAUDE_claude_3_5_sonnet_20240620.maxSubarraySum(arr);
        assertEquals(expected, actual);
    }

    @Test
    void testSubarrayAtTheStart() {
        int[] arr = {6, -1, -2, 1, 2};
        int expected = 6;
        int actual = Task145_CLAUDE_claude_3_5_sonnet_20240620.maxSubarraySum(arr);
        assertEquals(expected, actual);
    }
}
