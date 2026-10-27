def concatenate_strings(strings):
    return ''.join(strings)

def get_user_input():
    strings = []
    count = int(input("Enter number of strings: "))
    for i in range(count):
        s = input(f"Enter string {i+1}: ")
        strings.append(s)
    return strings

def main():
    strings = get_user_input()
    result = concatenate_strings(strings)
    print(result)

if __name__ == "__main__":
    main()