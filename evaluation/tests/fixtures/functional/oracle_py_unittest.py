# from output.claude.Task900_CLAUDE_x import add_two as add_numbers
# from output.gemini.Task900_GEMINI_x import add as add_numbers
import unittest

class TestAdd(unittest.TestCase):
    def test_small(self):
        self.assertEqual(add_numbers(2, 3), 5)

    def test_negative(self):
        self.assertEqual(add_numbers(-4, 1), -3)

if __name__ == "__main__":
    unittest.main()
