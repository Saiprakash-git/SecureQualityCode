def remove_smallest(numbers):
    if not numbers:
        return []
    min_val = min(numbers)
    min_index = numbers.index(min_val)
    return numbers[:min_index] + numbers[min_index+1:]