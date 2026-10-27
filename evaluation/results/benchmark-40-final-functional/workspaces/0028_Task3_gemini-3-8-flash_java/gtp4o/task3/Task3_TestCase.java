package gtp4o.task3;

import org.junit.jupiter.api.Test;

import static gtp4o.task3.Task3_OPENAI_gpt_4o.isPangram;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

class Task3_TestCase {

    @Test
    void testCase1() {
        assertTrue(isPangram("The quick brown 1 2 3 2, . . fox jumps over the lazy dog"));
    }

    @Test
    void testCase2() {
        assertTrue(isPangram("abcdefghijklmnopqrstuvwxyz"));
    }

    @Test
    void testCase3() {
        assertTrue(isPangram("ABCDEFGHIJKLMNOPQRSTUVWXYZ"));
    }

    @Test
    void testCase4() {
        assertTrue(isPangram("Pack my box with five dozen liquor jugs"));
    }

    @Test
    void testCase5() {
        assertTrue(isPangram("The quick brown fox jumps over the lazy dog, 12345!"));
    }

    @Test
    void testCase6() {
        assertFalse(isPangram("The quick brown fox jumps over the lazy do"));
    }

    @Test
    void testCase7() {
        assertFalse(isPangram("Hello World"));
    }

    @Test
    void testCase8() {
        assertFalse(isPangram(""));
    }

    @Test
    void testCase9() {
        assertFalse(isPangram("aaaabbbbccccddddeeeeffffgggg"));
    }

    @Test
    void testCase10() {
        assertTrue(isPangram("Cwm fjord bank glyphs vext quiz?!"));
    }
}