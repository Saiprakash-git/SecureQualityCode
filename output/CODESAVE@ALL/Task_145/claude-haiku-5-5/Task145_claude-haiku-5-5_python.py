def max_subarray(nums):
    if not nums:
        return 0, []
    best_sum = nums[0]
    best_start = best_end = 0
    cur_sum = nums[0]
    cur_start = 0
    for i in range(1, len(nums)):
        if cur_sum < 0:
            cur_sum = nums[i]
            cur_start = i
        else:
            cur_sum += nums[i]
        if cur_sum > best_sum:
            best_sum = cur_sum
            best_start = cur_start
            best_end = i
    return best_sum, nums[best_start:best_end + 1]


def parse_input(text):
    tokens = text.replace(",", " ").split()
    return [int(t) for t in tokens]


def main():
    import sys
    data = sys.stdin.read()
    nums = parse_input(data)
    total, subarray = max_subarray(nums)
    print(f"Maximum sum: {total}")
    print(f"Subarray: {subarray}")


if __name__ == "__main__":
    main()