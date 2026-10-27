def find_even_index(arr):
    right_sum = sum(arr)
    left_sum = 0

    for index, value in enumerate(arr):
        right_sum -= value
        if left_sum == right_sum:
            return index
        left_sum += value

    return -1