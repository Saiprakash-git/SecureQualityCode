def to_jaden_case(string):
    if string is None:
        return None
    return " ".join(word.capitalize() for word in string.split(" "))