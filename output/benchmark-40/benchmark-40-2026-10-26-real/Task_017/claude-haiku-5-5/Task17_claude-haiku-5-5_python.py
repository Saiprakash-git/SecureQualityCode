def to_jaden_case(s):
    if s is None or s == "":
        return None
    words = s.split(" ")
    return " ".join(word[:1].upper() + word[1:].lower() for word in words)