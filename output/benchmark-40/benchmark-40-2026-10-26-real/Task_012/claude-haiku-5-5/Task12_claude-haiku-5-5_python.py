def high(x):
    best_word = ""
    best_score = -1
    for word in x.split():
        score = sum(ord(c) - ord('a') + 1 for c in word)
        if score > best_score:
            best_score = score
            best_word = word
    return best_word