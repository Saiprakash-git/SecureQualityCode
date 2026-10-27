def is_pangram(s):
    return set("abcdefghijklmnopqrstuvwxyz") <= set(s.lower())