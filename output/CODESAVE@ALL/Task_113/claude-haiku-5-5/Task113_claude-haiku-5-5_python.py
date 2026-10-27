def concatenate_strings(strings):
    return "".join(strings)


def read_strings_from_input():
    count = int(input("How many strings do you want to concatenate? ").strip())
    strings = []
    for i in range(count):
        strings.append(input(f"Enter string {i + 1}: "))
    return strings


def main():
    strings = read_strings_from_input()
    result = concatenate_strings(strings)
    print("Concatenated result:", result)


if __name__ == "__main__":
    main()