def max_subarray(arr):
    if not arr:
        return []
    max_sum = current_sum = arr[0]
    start = end = temp_start = 0
    for i in range(1, len(arr)):
        if current_sum + arr[i] < arr[i]:
            current_sum = arr[i]
            temp_start = i
        else:
            current_sum += arr[i]
        if current_sum > max_sum:
            max_sum = current_sum
            start = temp_start
            end = i
    return arr[start:end+1]