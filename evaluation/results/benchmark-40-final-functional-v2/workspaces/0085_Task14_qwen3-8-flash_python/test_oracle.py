from adapter_bindings import *  # adapter
import unittest



class TestFindEvenIndex(unittest.TestCase):

    def test_case1(self):
        arr = [1, 2, 3, 4, 3, 2, 1]
        expected = 3
        self.assertEqual(find_even_index(arr), expected)

    def test_case2(self):
        arr = [1, 100, 50, -51, 1, 1]
        expected = 1
        self.assertEqual(find_even_index(arr), expected)

    def test_case3(self):
        arr = [20, 10, -80, 10, 10, 15, 35]
        expected = 0
        self.assertEqual(find_even_index(arr), expected)

    def test_case4(self):
        arr = [1, 2, 3, 4, 5, 6]
        expected = -1
        self.assertEqual(find_even_index(arr), expected)

    def test_case5(self):
        arr = [10, -10, 5, 5, 5]
        expected = 3
        self.assertEqual(find_even_index(arr), expected)

    def test_case6(self):
        arr = [-1, -2, -3, -4, -3, -2, -1]
        expected = 3
        self.assertEqual(find_even_index(arr), expected)

    def test_case7(self):
        arr = [0, 0, 0, 0, 0]
        expected = 0
        self.assertEqual(find_even_index(arr), expected)

    def test_case8(self):
        arr = [10, 20, 30, 40, 50, 60]
        expected = -1
        self.assertEqual(find_even_index(arr), expected)

    def test_case9(self):
        arr = [1, 1, 1, 1, 1, 1, 1, 6]
        expected = 6
        self.assertEqual(find_even_index(arr), expected)

    def test_case10(self):
        arr = [1, 0, -1, 0, 1, 0, -1, 0, 1, 0, -1, 0]
        expected = 3
        self.assertEqual(find_even_index(arr), expected)

if __name__ == "__main__":
    unittest.main()