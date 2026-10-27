from typing import List
import sys


def sort_integers(arr: List[int]) -> List[int]:
    return sorted(arr)


def parse_and_sort(input_string: str) -> List[int]:
    clean_str = input_string.replace(",", " ")
    numbers = [int(token) for token in clean_str.split()]
    return sort_integers(numbers)


if __name__ == "__main__":
    raw_input_data = sys.stdin.read().strip()
    if raw_input_data:
        sorted_result = parse_and_sort(raw_input_data)
        print(sorted_result)