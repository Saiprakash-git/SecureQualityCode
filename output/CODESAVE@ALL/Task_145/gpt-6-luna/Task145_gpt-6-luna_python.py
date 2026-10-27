import ast
import re
import sys


def maximum_subarray(numbers):
    """Return the maximum sum and the contiguous subarray that produces it."""
    if not numbers:
        return 0, []

    current_sum = best_sum = numbers[0]
    current_start = best_start = best_end = 0

    for index in range(1, len(numbers)):
        if numbers[index] > current_sum + numbers[index]:
            current_sum = numbers[index]
            current_start = index
        else:
            current_sum += numbers[index]

        if current_sum > best_sum:
            best_sum = current_sum
            best_start = current_start
            best_end = index

    return best_sum, numbers[best_start : best_end + 1]


def main():
    text = sys.stdin.read().strip()
    if not text:
        return

    try:
        parsed = ast.literal_eval(text)
        numbers = parsed if isinstance(parsed, (list, tuple)) else [parsed]
    except (ValueError, SyntaxError):
        numbers = [int(value) for value in re.findall(r"-?\d+", text)]

    best_sum, subarray = maximum_subarray(numbers)
    print(best_sum)
    print(*subarray)


if __name__ == "__main__":
    main()