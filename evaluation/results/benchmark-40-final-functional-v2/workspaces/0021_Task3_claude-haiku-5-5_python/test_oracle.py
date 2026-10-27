from adapter_bindings import *  # adapter
import unittest

import unittest

class TestPangram(unittest.TestCase):

    def test_case1(self):
        self.assertTrue(is_pangram("The quick 1 1 2 , , Z brown fox jumps over the lay dog"))

    def test_case2(self):
        self.assertTrue(is_pangram("abcdefghijklmnopqrstuvwxyz"))

    def test_case3(self):
        self.assertTrue(is_pangram("ABCDEFGHIJKLMNOPQRSTUVWXYZ"))

    def test_case4(self):
        self.assertTrue(is_pangram("Pack my box with five dozen liquor jugs"))

    def test_case5(self):
        self.assertTrue(is_pangram("The quick brown fox jumps over the lazy dog, 12345!"))

    def test_case6(self):
        self.assertFalse(is_pangram("The quick brown fox jumps over the lazy do"))

    def test_case7(self):
        self.assertFalse(is_pangram("Hello"))

    def test_case8(self):
        self.assertFalse(is_pangram(""))

    def test_case9(self):
        self.assertFalse(is_pangram("aaaaabbbbbcccccdddddeeeee"))

    def test_case10(self):
        self.assertTrue(is_pangram("Sphinx of black quartz, judge my vow."))

if __name__ == '__main__':
    unittest.main()
