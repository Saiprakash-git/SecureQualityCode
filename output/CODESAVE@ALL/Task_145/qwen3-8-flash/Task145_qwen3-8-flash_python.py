import sys
import re


def max_subarray(nums):
    if not nums:
        return 0, []

    max_sum = nums[0]
    current_sum = nums[0]
    start = 0
    end = 0
    temp_start = 0

    for i in range(1, len(nums)):
        if current_sum + nums[i] < nums[i]:
            current_sum = nums[i]
            temp_start = i
        else:
            current_sum += nums[i]

        if current_sum > max_sum:
            max_sum = current_sum
            start = temp_start
            end = i

    return max_sum, nums[start:end + 1]


def parse_integers(text):
    return [int(x) for x in re.findall(r"-?\d+", text)]


def main():
    data = sys.stdin.read().strip()
    nums = parse_integers(data) if data else []
    max_sum, subarray = max_subarray(nums)
    print(max_sum)
    print(subarray)


if __name__ == "__main__":
    main()