def binary_array_to_int(arr):
    if not arr:
        return 0
    return int(''.join(str(bit) for bit in arr), 2)