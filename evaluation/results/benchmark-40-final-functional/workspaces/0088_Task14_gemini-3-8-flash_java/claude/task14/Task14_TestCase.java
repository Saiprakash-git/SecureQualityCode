package claude.task14;

import org.junit.jupiter.api.Test;

import static claude.task14.Task14_CLAUDE_claude_3_5_sonnet_20240620.findEvenIndex;
import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;

public class Task14_TestCase {
    @Test
    void testCase1() {
        int[] arr = {1, 2, 3, 4, 3, 2, 1};
        int expected = 3;
        assertEquals(expected, findEvenIndex(arr));
    }

    @Test
    void testCase2() {
        int[] arr = {1, 100, 50, -51, 1, 1};
        int expected = 1;
        assertEquals(expected, findEvenIndex(arr));
    }

    @Test
    void testCase3() {
        int[] arr = {20, 10, -80, 10, 10, 15, 35};
        int expected = 0;
        assertEquals(expected, findEvenIndex(arr));
    }

    @Test
    void testCase4() {
        int[] arr = {1, 2, 3, 4, 5, 6};
        int expected = -1;
        assertEquals(expected, findEvenIndex(arr));
    }

    @Test
    void testCase5() {
        int[] arr = {10, -10, 5, 5, 5};
        int expected = 3;
        assertEquals(expected, findEvenIndex(arr));
    }

    @Test
    void testCase6() {
        int[] arr = {-1, -2, -3, -4, -3, -2, -1};
        int expected = 3;
        assertEquals(expected, findEvenIndex(arr));
    }

    @Test
    void testCase7() {
        int[] arr = {0, 0, 0, 0, 0};
        int expected = 0;
        assertEquals(expected, findEvenIndex(arr));
    }

    @Test
    void testCase8() {
        int[] arr = {10, 20, 30, 40, 50, 60};
        int expected = -1;
        assertEquals(expected, findEvenIndex(arr));
    }

    @Test
    void testCase9() {
        int[] arr = {1, 1, 1, 1, 1, 1,1, 6};
        int expected = 6;
        assertEquals(expected, findEvenIndex(arr));
    }

    @Test
    void testCase10() {
        int[] arr = {1, 0, -1,0, 1, 0, -1,0,1, 0, -1,0};
        int expected = 3;
        assertEquals(expected, findEvenIndex(arr));
    }
}
