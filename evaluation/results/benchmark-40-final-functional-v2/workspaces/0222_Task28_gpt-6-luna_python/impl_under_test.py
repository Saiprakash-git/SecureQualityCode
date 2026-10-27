def likes(names):
    count = len(names)
    if count == 0:
        return "no one likes this"
    if count == 1:
        return f"{names[0]} likes this"
    if count == 2:
        return f"{names[0]} and {names[1]} like this"
    if count == 3:
        return f"{names[0]}, {names[1]} and {names[2]} like this"
    return f"{names[0]}, {names[1]} and {count - 2} others like this"