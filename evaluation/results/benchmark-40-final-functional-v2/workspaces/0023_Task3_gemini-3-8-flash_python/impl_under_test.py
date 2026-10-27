import string


def is_pangram(s: str) -> bool:
    return set(string.ascii_lowercase).issubset(set(s.lower()))