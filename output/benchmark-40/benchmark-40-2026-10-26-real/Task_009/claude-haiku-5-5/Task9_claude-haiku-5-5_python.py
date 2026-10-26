def odd_or_even(arr):
    total = sum(arr) if arr else 0
    return "odd" if total % 2 != 0 else "even"