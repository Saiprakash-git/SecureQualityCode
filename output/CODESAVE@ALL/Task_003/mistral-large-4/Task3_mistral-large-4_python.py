def is_pangram(s):
    return set('abcdefghijklmnopqrstuvwxyz').issubset({c.lower() for c in s if c.isalpha()})