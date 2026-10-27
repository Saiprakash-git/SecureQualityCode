def high(s: str) -> str:
    return max(s.split(), key=lambda word: sum(ord(c) - 96 for c in word))