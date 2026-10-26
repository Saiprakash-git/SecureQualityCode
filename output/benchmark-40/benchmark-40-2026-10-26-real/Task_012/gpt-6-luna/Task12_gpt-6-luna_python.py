def high(x):
    return max(x.split(), key=lambda word: sum(ord(letter) - ord("a") + 1 for letter in word))