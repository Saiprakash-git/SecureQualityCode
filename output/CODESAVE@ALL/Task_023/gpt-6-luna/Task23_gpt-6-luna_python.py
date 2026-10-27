from math import isqrt

def find_nb(m):
    if m < 0:
        return -1

    total = isqrt(m)
    if total * total != m:
        return -1

    discriminant = 1 + 8 * total
    root = isqrt(discriminant)
    if root * root != discriminant:
        return -1

    n = (root - 1) // 2
    return n if n * (n + 1) // 2 == total else -1