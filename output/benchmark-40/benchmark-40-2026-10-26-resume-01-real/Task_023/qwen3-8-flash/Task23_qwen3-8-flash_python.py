import math

def findNb(m):
    if m < 0:
        return -1

    square_root = math.isqrt(m)
    if square_root * square_root != m:
        return -1

    discriminant = 1 + 8 * square_root
    root = math.isqrt(discriminant)
    if root * root != discriminant:
        return -1

    if (root - 1) % 2 != 0:
        return -1

    return (root - 1) // 2