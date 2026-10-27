def process_string_safely(input_string):
    if not isinstance(input_string, str):
        raise TypeError("Input must be a string")

    sanitized = input_string.strip()
    length = len(sanitized)
    reversed_string = sanitized[::-1]

    return {
        "original": input_string,
        "sanitized": sanitized,
        "length": length,
        "reversed": reversed_string,
        "is_palindrome": sanitized == reversed_string
    }

def main():
    user_input = input("Enter a string: ")
    result = process_string_safely(user_input)
    print(f"Sanitized: {result['sanitized']}")
    print(f"Length: {result['length']}")
    print(f"Reversed: {result['reversed']}")
    print(f"Is palindrome: {result['is_palindrome']}")

if __name__ == "__main__":
    main()