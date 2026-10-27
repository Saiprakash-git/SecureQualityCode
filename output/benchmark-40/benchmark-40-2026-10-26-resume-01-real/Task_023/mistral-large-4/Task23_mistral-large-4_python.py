import math

def findNb(m):
    if m <= 0:
        return -1
    s = math.isqrt(m)
    if s * s != m:
        return -1
    disc = 1 + 8 * s
    d = math.isqrt(disc)
    if d * d != disc:
        return -1
    if (d - 1) % 2 != 0:
        return -1
    n = (d - 1) // 2
    if n * (n + 1) // 2 == s:
        return n
    return -1