package claude.task900;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static claude.task900.Task900_CLAUDE_x.add;

public class Task900_TestCase {
    @Test
    void small() { assertEquals(5, add(2, 3)); }

    @Test
    void negative() { assertEquals(-3, add(-4, 1)); }
}
