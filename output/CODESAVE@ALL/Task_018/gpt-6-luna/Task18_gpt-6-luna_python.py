def binary_array_to_number(arr):
    return int("".join(map(str, arr)), 2) if arr else 0