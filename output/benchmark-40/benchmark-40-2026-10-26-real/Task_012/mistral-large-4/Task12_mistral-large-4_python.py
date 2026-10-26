def highest_scoring_word(s):
    return max(s.split(), key=lambda word: sum(ord(c) - ord('a') + 1 for c in word))