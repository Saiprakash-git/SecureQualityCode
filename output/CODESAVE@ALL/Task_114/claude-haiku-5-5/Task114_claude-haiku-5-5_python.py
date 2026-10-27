def copy_string(s):
    return str(s)

def concatenate_strings(*parts, separator=""):
    return separator.join(str(p) for p in parts)

def reverse_string(s):
    return s[::-1]

def to_upper(s):
    return s.upper()

def to_lower(s):
    return s.lower()

def capitalize_words(s):
    return " ".join(word.capitalize() for word in s.split())

def count_occurrences(s, sub):
    return s.count(sub)

def replace_substring(s, old, new):
    return s.replace(old, new)

def remove_whitespace(s):
    return "".join(s.split())

def is_palindrome(s):
    cleaned = "".join(ch.lower() for ch in s if ch.isalnum())
    return cleaned == cleaned[::-1]

def substring(s, start, end=None):
    return s[start:end]

def pad_string(s, width, fill=" ", align="left"):
    if align == "right":
        return s.rjust(width, fill)
    if align == "center":
        return s.center(width, fill)
    return s.ljust(width, fill)

def split_string(s, delimiter=None):
    return s.split(delimiter)

def join_strings(items, separator=""):
    return separator.join(str(i) for i in items)

def char_frequency(s):
    freq = {}
    for ch in s:
        freq[ch] = freq.get(ch, 0) + 1
    return freq

def trim_string(s, chars=None):
    return s.strip(chars)

def repeat_string(s, times):
    return s * times

def main():
    original = "  Hello, World!  "
    print("Original:", repr(original))
    copied = copy_string(original)
    print("Copied:", repr(copied))
    print("Trimmed:", repr(trim_string(original)))
    print("Concatenated:", concatenate_strings("Hello", "World", separator=" "))
    print("Reversed:", reverse_string("Python"))
    print("Upper:", to_upper("python"))
    print("Lower:", to_lower("PYTHON"))
    print("Capitalized:", capitalize_words("the quick brown fox"))
    print("Count of 'l':", count_occurrences("hello world", "l"))
    print("Replaced:", replace_substring("I like cats", "cats", "dogs"))
    print("No whitespace:", remove_whitespace("a b c d"))
    print("Palindrome 'A man, a plan, a canal: Panama':",
          is_palindrome("A man, a plan, a canal: Panama"))
    print("Substring [0:5]:", substring("abcdefgh", 0, 5))
    print("Padded right:", repr(pad_string("42", 6, "0", "right")))
    print("Centered:", repr(pad_string("hi", 6, "*", "center")))
    print("Split:", split_string("a,b,c", ","))
    print("Joined:", join_strings([1, 2, 3], "-"))
    print("Char frequency:", char_frequency("banana"))
    print("Repeated:", repeat_string("ab", 3))

if __name__ == "__main__":
    main()