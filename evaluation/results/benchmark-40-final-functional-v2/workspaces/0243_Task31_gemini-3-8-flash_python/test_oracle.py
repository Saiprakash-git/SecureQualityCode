from adapter_bindings import *  # adapter
import unittest


class TestMakeComplement(unittest.TestCase):

    def test_complementary_strand_1(self):
        # Test case with standard input
        self.assertEqual(make_complement("ATTGC"), "TAACG")

    def test_complementary_strand_2(self):
        # Test case with standard input
        self.assertEqual(make_complement("GTAT"), "CATA")

    def test_single_character_a(self):
        # Test case with a single character 'A'
        self.assertEqual(make_complement("A"), "T")

    def test_single_character_t(self):
        # Test case with a single character 'T'
        self.assertEqual(make_complement("T"), "A")

    def test_single_character_c(self):
        # Test case with a single character 'C'
        self.assertEqual(make_complement("C"), "G")

    def test_single_character_g(self):
        # Test case with a single character 'G'
        self.assertEqual(make_complement("G"), "C")

    def test_repeating_characters(self):
        # Test case with repeating characters
        self.assertEqual(make_complement("AAAA"), "TTTT")

    def test_alternating_characters(self):
        # Test case with alternating characters
        self.assertEqual(make_complement("TATA"), "ATAT")

    def test_long_strand(self):
        # Test case with a long DNA strand
        self.assertEqual(make_complement("GCGCGCGCGC"), "CGCGCGCGCG")

    def test_mixed_case(self):
        # Test case with a mixed-case DNA strand
        self.assertEqual(make_complement("attgc".upper()), "TAACG")

if __name__ == '__main__':
    unittest.main()