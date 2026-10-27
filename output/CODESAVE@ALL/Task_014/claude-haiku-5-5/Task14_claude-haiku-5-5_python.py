def find_even_index(arr):
    total = sum(arr)
    left = 0
    for i, value in enumerate(arr):
        right = total - left - value
        if left == right:
            return i
        left += value
    return -1