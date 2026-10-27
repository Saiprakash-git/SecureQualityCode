package claude.task15;

import org.junit.jupiter.api.Test;

import static claude.task15.Task15_CLAUDE_claude_3_5_sonnet_20240620.towerBuilder;
import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;

public class Task15_TestCase {

    @Test
    void testOneFloor() {
        String[] expected = {"*"};
        assertArrayEquals(expected, towerBuilder(1));
    }

    @Test
    void testTwoFloors() {
        String[] expected = {
                " * ",
                "***"
        };
        assertArrayEquals(expected, towerBuilder(2));
    }

    @Test
    void testThreeFloors() {
        String[] expected = {
                "  *  ",
                " *** ",
                "*****"
        };
        assertArrayEquals(expected, towerBuilder(3));
    }

    @Test
    void testFourFloors() {
        String[] expected = {
                "   *   ",
                "  ***  ",
                " ***** ",
                "*******"
        };
        assertArrayEquals(expected, towerBuilder(4));
    }

    @Test
    void testFiveFloors() {
        String[] expected = {
                "    *    ",
                "   ***   ",
                "  *****  ",
                " ******* ",
                "*********"
        };
        assertArrayEquals(expected, towerBuilder(5));
    }

    @Test
    void testSixFloors() {
        String[] expected = {
                "     *     ",
                "    ***    ",
                "   *****   ",
                "  *******  ",
                " ********* ",
                "***********"
        };
        assertArrayEquals(expected, towerBuilder(6));
    }

    @Test
    void testSevenFloors() {
        String[] expected = {
                "      *      ",
                "     ***     ",
                "    *****    ",
                "   *******   ",
                "  *********  ",
                " *********** ",
                "*************"
        };
        assertArrayEquals(expected, towerBuilder(7));
    }

    @Test
    void testEightFloors() {
        String[] expected = {
                "       *       ",
                "      ***      ",
                "     *****     ",
                "    *******    ",
                "   *********   ",
                "  ***********  ",
                " ************* ",
                "***************"
        };
        assertArrayEquals(expected, towerBuilder(8));
    }

    @Test
    void testNineFloors() {
        String[] expected = {
                "        *        ",
                "       ***       ",
                "      *****      ",
                "     *******     ",
                "    *********    ",
                "   ***********   ",
                "  *************  ",
                " *************** ",
                "*****************"
        };
        assertArrayEquals(expected, towerBuilder(9));
    }

    @Test
    void testTenFloors() {
        String[] expected = {
                "         *         ",
                "        ***        ",
                "       *****       ",
                "      *******      ",
                "     *********     ",
                "    ***********    ",
                "   *************   ",
                "  ***************  ",
                " ***************** ",
                "*******************"
        };
        assertArrayEquals(expected, towerBuilder(10));
    }
}
