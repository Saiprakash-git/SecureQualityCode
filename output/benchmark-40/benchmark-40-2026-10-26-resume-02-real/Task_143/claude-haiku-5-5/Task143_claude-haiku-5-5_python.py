def sort_ascending(arr):
    return sorted(arr)


def parse_input(text):
    return [int(x) for x in text.replace(",", " ").split()]


def main():
    import sys
    data = sys.stdin.read()
    numbers = parse_input(data)
    result = sort_ascending(numbers)
    print(" ".join(map(str, result)))


if __name__ == "__main__":
    main()