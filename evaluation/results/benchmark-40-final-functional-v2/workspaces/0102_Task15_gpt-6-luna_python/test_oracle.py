from adapter_bindings import *  # adapter
import unittest
def tower_builder(n):
    # Generates a list of strings representing the tower with n floors
    return [
        ' ' * (n - i - 1) + '*' * (2 * i + 1) + ' ' * (n - i - 1)
        for i in range(n)
    ]

class TestTowerBuilder(unittest.TestCase):

    def test_one_floor(self):
        expected = ["*"]
        self.assertEqual(tower_builder(1), expected)

    def test_two_floors(self):
        expected = [
            " * ",
            "***"
        ]
        self.assertEqual(tower_builder(2), expected)

    def test_three_floors(self):
        expected = [
            "  *  ",
            " *** ",
            "*****"
        ]
        self.assertEqual(tower_builder(3), expected)

    def test_four_floors(self):
        expected = [
            "   *   ",
            "  ***  ",
            " ***** ",
            "*******"
        ]
        self.assertEqual(tower_builder(4), expected)

    def test_five_floors(self):
        expected = [
            "    *    ",
            "   ***   ",
            "  *****  ",
            " ******* ",
            "*********"
        ]
        self.assertEqual(tower_builder(5), expected)

    def test_six_floors(self):
        expected = [
            "     *     ",
            "    ***    ",
            "   *****   ",
            "  *******  ",
            " ********* ",
            "***********"
        ]
        self.assertEqual(tower_builder(6), expected)

    def test_seven_floors(self):
        expected = [
            "      *      ",
            "     ***     ",
            "    *****    ",
            "   *******   ",
            "  *********  ",
            " *********** ",
            "*************"
        ]
        self.assertEqual(tower_builder(7), expected)

    def test_eight_floors(self):
        expected = [
            "       *       ",
            "      ***      ",
            "     *****     ",
            "    *******    ",
            "   *********   ",
            "  ***********  ",
            " ************* ",
            "***************"
        ]
        self.assertEqual(tower_builder(8), expected)

    def test_nine_floors(self):
        expected = [
            "        *        ",
            "       ***       ",
            "      *****      ",
            "     *******     ",
            "    *********    ",
            "   ***********   ",
            "  *************  ",
            " *************** ",
            "*****************"
        ]
        self.assertEqual(tower_builder(9), expected)

    def test_ten_floors(self):
        expected = [
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
        ]
        self.assertEqual(tower_builder(10), expected)

if __name__ == "__main__":
    unittest.main()