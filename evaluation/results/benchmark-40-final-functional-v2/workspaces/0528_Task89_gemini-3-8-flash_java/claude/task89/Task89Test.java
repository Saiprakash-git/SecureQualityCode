package claude.task89;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

public class Task89Test {

    @Test
    void testExactMatch() {
        assertTrue(Task89_CLAUDE_claude_3_5_sonnet_20240620.checkPassword("secure123", "secure123"));
    }

    @Test
    void testDifferentLength() {
        assertFalse(Task89_CLAUDE_claude_3_5_sonnet_20240620.checkPassword("secure123", "secure12"));
    }

    @Test
    void testOneCharacterMismatch() {
        assertFalse(Task89_CLAUDE_claude_3_5_sonnet_20240620.checkPassword("secure123", "secure124"));
    }

    @Test
    void testCompletelyDifferent() {
        assertFalse(Task89_CLAUDE_claude_3_5_sonnet_20240620.checkPassword("password", "12345678"));
    }

    @Test
    void testEmptyPasswordMatch() {
        assertTrue(Task89_CLAUDE_claude_3_5_sonnet_20240620.checkPassword("", ""));
    }

    @Test
    void testEmptyStoredPassword() {
        assertFalse(Task89_CLAUDE_claude_3_5_sonnet_20240620.checkPassword("", "abc"));
    }

    @Test
    void testEmptyEnteredPassword() {
        assertFalse(Task89_CLAUDE_claude_3_5_sonnet_20240620.checkPassword("abc", ""));
    }
}
