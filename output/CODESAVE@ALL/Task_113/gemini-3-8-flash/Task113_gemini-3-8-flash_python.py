def concatenate_strings(strings: list[str], separator: str = "") -> str:
    return separator.join(strings)


def main():
    try:
        count = int(input("Enter the number of strings to concatenate: "))
    except ValueError:
        print("Invalid number entered.")
        return

    strings = []
    for i in range(count):
        s = input(f"Enter string #{i + 1}: ")
        strings.append(s)

    separator = input("Enter a separator (press Enter for none): ")
    result = concatenate_strings(strings, separator)
    print("Result:", result)


if __name__ == "__main__":
    main()