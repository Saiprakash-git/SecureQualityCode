def highest_scoring_word(s):
    words = s.split()
    if not words:
        return ""

    best_word = words[0]
    best_score = sum(ord(ch) - ord("a") + 1 for ch in best_word)

    for word in words[1:]:
        score = sum(ord(ch) - ord("a") + 1 for ch in word)
        if score > best_score:
            best_score = score
            best_word = word

    return best_word