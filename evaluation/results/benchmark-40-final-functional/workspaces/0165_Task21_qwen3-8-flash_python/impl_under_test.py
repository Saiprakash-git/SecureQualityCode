def remove_smallest(numbers):
    if numbers is None:
        return []

    values = list(numbers)

    if not values:
        return []

    index = values.index(min(values))
    return values[:index] + values[index + 1:]