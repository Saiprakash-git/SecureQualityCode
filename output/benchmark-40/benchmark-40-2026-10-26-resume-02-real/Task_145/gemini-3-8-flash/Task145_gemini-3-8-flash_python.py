from typing import List, Tuple

def find_max_subarray(nums: List[int]) -> Tuple[int, List[int]]:
    """
    Finds the contiguous subarray with the maximum sum using Kadane's Algorithm.

    Args:
        nums: A list of integers.

    Returns:
        A tuple containing (max_sum, subarray).
        Returns (0, []) if the input list is empty.
    """
    if not nums:
        return 0, []

    max_so_far = nums[0]
    current_max = nums[0]

    start = 0
    end = 0
    temp_start = 0

    for i in range(1, len(nums)):
        if nums[i] > current_max + nums[i]:
            current_max = nums[i]
            temp_start = i
        else:
            current_max += nums[i]

        if current_max > max_so_far:
            max_so_far = current_max
            start = temp_start
            end = i

    return max_so_far, nums[start:end + 1]


def main():
    sample_input = [-2, 1, -3, 4, -1, 2, 1, -5, 4]
    max_sum, subarray = find_max_subarray(sample_input)
    print(f"Input Array: {sample_input}")
    print(f"Maximum Sum: {max_sum}")
    print(f"Contiguous Subarray: {subarray}")


if __name__ == "__main__":
    main()