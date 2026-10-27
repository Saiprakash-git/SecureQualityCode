package claude.task25;
import org.junit.jupiter.api.Test;

import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import static org.junit.jupiter.api.Assertions.assertEquals;

public class Task25_TestCase {

    @Test
    void testCase1() {
        List<String> input = Collections.emptyList();
        List<String> expected = Collections.emptyList();
        assertEquals(expected, Task25_CLAUDE_claude_3_5_sonnet_20240620.number(input));
    }

    @Test
    void testCase2() {
        List<String> input = Collections.singletonList("Hello");
        List<String> expected = Collections.singletonList("1: Hello");
        assertEquals(expected, Task25_CLAUDE_claude_3_5_sonnet_20240620.number(input));
    }

    @Test
    void testCase3() {
        List<String> input = Arrays.asList("Hello", "World");
        List<String> expected = Arrays.asList("1: Hello", "2: World");
        assertEquals(expected, Task25_CLAUDE_claude_3_5_sonnet_20240620.number(input));
    }

    @Test
    void testCase4() {
        List<String> input = Arrays.asList("a", "b", "c");
        List<String> expected = Arrays.asList("1: a", "2: b", "3: c");
        assertEquals(expected, Task25_CLAUDE_claude_3_5_sonnet_20240620.number(input));
    }

    @Test
    void testCase5() {
        List<String> input = Arrays.asList("This is a long line", "Another long line");
        List<String> expected = Arrays.asList("1: This is a long line", "2: Another long line");
        assertEquals(expected, Task25_CLAUDE_claude_3_5_sonnet_20240620.number(input));
    }

    @Test
    void testCase6() {
        List<String> input = Arrays.asList("Line 1", "", "Line 3");
        List<String> expected = Arrays.asList("1: Line 1", "2: ", "3: Line 3");
        assertEquals(expected, Task25_CLAUDE_claude_3_5_sonnet_20240620.number(input));
    }

    @Test
    void testCase7() {
        List<String> input = Arrays.asList("!@#$%^", "&*()_+", "[]{}");
        List<String> expected = Arrays.asList("1: !@#$%^", "2: &*()_+", "3: []{}");
        assertEquals(expected, Task25_CLAUDE_claude_3_5_sonnet_20240620.number(input));
    }

    @Test
    void testCase8() {
        List<String> input = Arrays.asList("123", "456", "789");
        List<String> expected = Arrays.asList("1: 123", "2: 456", "3: 789");
        assertEquals(expected, Task25_CLAUDE_claude_3_5_sonnet_20240620.number(input));
    }

    @Test
    void testCase9() {
        List<String> input = Arrays.asList("  leading", "trailing  ", "  both  ");
        List<String> expected = Arrays.asList("1:   leading", "2: trailing  ", "3:   both  ");
        assertEquals(expected, Task25_CLAUDE_claude_3_5_sonnet_20240620.number(input));
    }

    @Test
    void testCase10() {
        List<String> input = IntStream.range(0, 1000)
                .mapToObj(i -> "line " + i)
                .collect(Collectors.toList());
        List<String> expected = IntStream.range(0, 1000)
                .mapToObj(i -> (i + 1) + ": line " + i)
                .collect(Collectors.toList());
        assertEquals(expected, Task25_CLAUDE_claude_3_5_sonnet_20240620.number(input));
    }
}
