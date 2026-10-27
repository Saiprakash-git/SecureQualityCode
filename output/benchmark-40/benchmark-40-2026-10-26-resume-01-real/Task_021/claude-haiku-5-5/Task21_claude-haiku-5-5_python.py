def remove_smallest(numbers):
    if not numbers:
        return []
    result = list(numbers)
    min_index = 0
    for i in range(1, len(result)):
        if result[i] < result[min_index]:
            min_index = i
    del result[min_index]
    return result

if __name__ == "__main__":
    print(remove_smallest([1, 2, 3, 4, 5]))
    print(remove_smallest([5, 3, 2, 1, 4]))
    print(remove_smallest([2, 2, 1, 2, 1]))