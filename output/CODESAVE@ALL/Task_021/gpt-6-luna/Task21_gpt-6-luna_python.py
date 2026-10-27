def remove_smallest(numbers):
    if not numbers:
        return []
    index = numbers.index(min(numbers))
    return numbers[:index] + numbers[index + 1:]