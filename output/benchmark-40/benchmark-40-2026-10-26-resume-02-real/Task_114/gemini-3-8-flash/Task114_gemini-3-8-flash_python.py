import re


def copy_string(source: str) -> str:
    """Returns an explicit copy of the provided string."""
    return str(source)


def concatenate_strings(*args: str, delimiter: str = "") -> str:
    """Concatenates multiple strings separated by an optional delimiter."""
    return delimiter.join(args)


def reverse_string(text: str) -> str:
    """Reverses the input string."""
    return text[::-1]


def to_uppercase(text: str) -> str:
    """Converts the input string to uppercase."""
    return text.upper()


def to_lowercase(text: str) -> str:
    """Converts the input string to lowercase."""
    return text.lower()


def toggle_case(text: str) -> str:
    """Inverts the case of each character in the string."""
    return text.swapcase()


def replace_substring(text: str, old_sub: str, new_sub: str) -> str:
    """Replaces all occurrences of a substring with another substring."""
    return text.replace(old_sub, new_sub)


def extract_substring(text: str, start: int, end: int) -> str:
    """Extracts a slice of the string from start index to end index."""
    return text[start:end]


def count_occurrences(text: str, sub: str) -> int:
    """Counts the occurrences of a substring within the text."""
    return text.count(sub)


def trim_whitespace(text: str) -> str:
    """Removes leading and trailing whitespace from the string."""
    return text.strip()


def pad_string(text: str, length: int, fill_char: str = " ", alignment: str = "left") -> str:
    """Pads a string to a specific length using the given character and alignment."""
    if len(fill_char) != 1:
        raise ValueError("fill_char must be exactly one character.")
    if alignment == "left":
        return text.ljust(length, fill_char)
    elif alignment == "right":
        return text.rjust(length, fill_char)
    elif alignment == "center":
        return text.center(length, fill_char)
    else:
        raise ValueError("alignment must be 'left', 'right', or 'center'.")


def is_palindrome(text: str) -> bool:
    """Checks whether the input string is a palindrome (ignoring non-alphanumeric and case)."""
    cleaned = re.sub(r"[^a-zA-Z0-9]", "", text).lower()
    return cleaned == cleaned[::-1]


def is_anagram(text1: str, text2: str) -> bool:
    """Determines whether two strings are anagrams of each other."""
    cleaned1 = sorted(re.sub(r"[^a-zA-Z0-9]", "", text1).lower())
    cleaned2 = sorted(re.sub(r"[^a-zA-Z0-9]", "", text2).lower())
    return cleaned1 == cleaned2


def count_vowels_and_consonants(text: str) -> dict:
    """Counts the number of vowels and consonants in the text."""
    vowels = set("aeiouAEIOU")
    vowel_count = 0
    consonant_count = 0
    for char in text:
        if char.isalpha():
            if char in vowels:
                vowel_count += 1
            else:
                consonant_count += 1
    return {"vowels": vowel_count, "consonants": consonant_count}


def count_words(text: str) -> int:
    """Counts total words in the text."""
    return len(text.split())


def truncate_string(text: str, max_length: int, suffix: str = "...") -> str:
    """Truncates the string if it exceeds max_length, appending a suffix."""
    if len(text) <= max_length:
        return text
    cutoff = max_length - len(suffix)
    if cutoff < 0:
        return suffix[:max_length]
    return text[:cutoff] + suffix


def demonstrate_string_operations() -> None:
    original = "Hello, World!"
    copied = copy_string(original)
    concatenated = concatenate_strings("Python", "String", "Operations", delimiter=" - ")
    reversed_str = reverse_string(original)
    upper_str = to_uppercase(original)
    lower_str = to_lowercase(original)
    toggled = toggle_case("pYtHoN")
    replaced = replace_substring("apple banana apple", "apple", "orange")
    substring = extract_substring(original, 7, 12)
    occurrences = count_occurrences("banana", "a")
    trimmed = trim_whitespace("   Spaced Out   ")
    left_padded = pad_string("Code", 10, fill_char="*", alignment="left")
    center_padded = pad_string("Code", 10, fill_char="*", alignment="center")
    palindrome_check = is_palindrome("A man, a plan, a canal: Panama")
    anagram_check = is_anagram("Listen", "Silent")
    v_c_counts = count_vowels_and_consonants(original)
    word_count = count_words("The quick brown fox jumps over the lazy dog.")
    truncated = truncate_string("This is a long sentence that needs truncating.", 20)

    print(f"Original: {original}")
    print(f"Copied: {copied}")
    print(f"Concatenated: {concatenated}")
    print(f"Reversed: {reversed_str}")
    print(f"Uppercase: {upper_str}")
    print(f"Lowercase: {lower_str}")
    print(f"Toggled Case: {toggled}")
    print(f"Replaced: {replaced}")
    print(f"Extracted Substring (7-12): {substring}")
    print(f"Occurrences of 'a' in 'banana': {occurrences}")
    print(f"Trimmed: '{trimmed}'")
    print(f"Left Padded: '{left_padded}'")
    print(f"Center Padded: '{center_padded}'")
    print(f"Is 'A man, a plan, a canal: Panama' Palindrome?: {palindrome_check}")
    print(f"Are 'Listen' and 'Silent' Anagrams?: {anagram_check}")
    print(f"Vowels and Consonants: {v_c_counts}")
    print(f"Word Count: {word_count}")
    print(f"Truncated: {truncated}")


if __name__ == "__main__":
    demonstrate_string_operations()