def number_lines(lines):
    return [f"{i}: {line}" for i, line in enumerate(lines, start=1)]

if __name__ == "__main__":
    print(number_lines([]))
    print(number_lines(["a", "b", "c"]))