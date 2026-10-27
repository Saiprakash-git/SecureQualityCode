import math

def find_nb(m):
    s = math.isqrt(m)
    if s * s != m:
        return -1
    
    d = math.isqrt(1 + 8 * s)
    if d * d != 1 + 8 * s:
        return -1
    
    n = (d - 1) // 2
    return n if n > 0 else -1

findNb = find_nb