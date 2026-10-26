def is_pangram(sentence):
    letters = set(ch.lower() for ch in sentence if ch.isascii() and ch.isalpha())
    return len(letters) == 26


if __name__ == "__main__":
    import sys
    text = sys.stdin.read() if not sys.argv[1:] else " ".join(sys.argv[1:])
    print(is_pangram(text))