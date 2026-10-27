def remove_smallest(numbers):
    if not numbers:
        return []
    min_idx = numbers.index(min(numbers))
    return numbers[:min_idx] + numbers[min_idx + 1:]