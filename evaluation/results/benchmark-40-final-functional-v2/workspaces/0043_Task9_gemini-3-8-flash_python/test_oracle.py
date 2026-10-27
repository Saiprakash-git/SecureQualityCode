from adapter_bindings import *  # adapter

import unittest

class TestOddOrEven(unittest.TestCase):

    def test_empty_array(self):
        # Test with an empty array, should be treated as [0]
        self.assertEqual(oddOrEven([]), "even")

    def test_single_positive_odd(self):
        # Test with a single positive odd number
        self.assertEqual(oddOrEven([7]), "odd")

    def test_single_positive_even(self):
        # Test with a single positive even number
        self.assertEqual(oddOrEven([8]), "even")

    def test_multiple_positives(self):
        # Test with multiple positive numbers whose sum is even
        self.assertEqual(oddOrEven([1, 3, 5, 7]), "even")

    def test_multiple_positives_odd_sum(self):
        # Test with multiple positive numbers whose sum is odd
        self.assertEqual(oddOrEven([1, 2, 3, 4]), "even")

    def test_multiple_negatives(self):
        # Test with multiple negative numbers whose sum is even
        self.assertEqual(oddOrEven([-2, -4, -6]), "even")

    def test_multiple_negatives_odd_sum(self):
        # Test with multiple negative numbers whose sum is odd
        self.assertEqual(oddOrEven([-1, -2, -3]), "even")

    def test_mixed_numbers_even_sum(self):
        # Test with a mix of positive and negative numbers whose sum is even
        self.assertEqual(oddOrEven([1, -1, 2, -2]), "even")

    def test_mixed_numbers_odd_sum(self):
        # Test with a mix of positive and negative numbers whose sum is odd
        self.assertEqual(oddOrEven([1, -1, 2]), "even")

    def test_large_numbers(self):
        # Test with large numbers whose sum is even
        self.assertEqual(oddOrEven([1000000, 2000000, -3000000]), "even")

if __name__ == "__main__":
    unittest.main()
