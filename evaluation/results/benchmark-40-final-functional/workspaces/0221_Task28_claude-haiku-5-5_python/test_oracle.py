from adapter_bindings import *  # adapter
import unittest

class TestTask28(unittest.TestCase):

    def test_no_likes(self):
        # Test when there are no names in the array
        self.assertEqual(likes([]), "no one likes this")

    def test_one_like(self):
        # Test when there is one name in the array
        self.assertEqual(likes(["Peter"]), "Peter likes this")

    def test_two_likes(self):
        # Test when there are two names in the array
        self.assertEqual(likes(["Jacob", "Alex"]), "Jacob and Alex like this")

    def test_three_likes(self):
        # Test when there are three names in the array
        self.assertEqual(likes(["Max", "John", "Mark"]), "Max, John and Mark like this")

    def test_four_likes(self):
        # Test when there are four names in the array
        self.assertEqual(likes(["Alex", "Jacob", "Mark", "Max"]), "Alex, Jacob and 2 others like this")

    def test_five_likes(self):
        # Test when there are five names in the array
        self.assertEqual(likes(["John", "Paul", "George", "Ringo", "Brian"]), "John, Paul and 3 others like this")

    def test_six_likes(self):
        # Test when there are six names in the array
        self.assertEqual(likes(["Alice", "Bob", "Charlie", "Dave", "Eve", "Frank"]), "Alice, Bob and 4 others like this")

    def test_names_with_spaces(self):
        # Test when names contain spaces
        self.assertEqual(likes(["Anna Marie", "John Doe", "Jane Smith", "Joe Public"]), "Anna Marie, John Doe and 2 others like this")

    def test_duplicate_names(self):
        # Test when names are duplicated in the array
        self.assertEqual(likes(["Sam", "Sam", "Alex"]), "Sam, Sam and Alex like this")

    def test_long_array(self):
        # Test with a long array of names
        names = ["Tom", "Dick", "Harry", "Sally", "Jane", "Susan", "Bill", "Bob"]
        self.assertEqual(likes(names), "Tom, Dick and 6 others like this")

if __name__ == '__main__':
    unittest.main()