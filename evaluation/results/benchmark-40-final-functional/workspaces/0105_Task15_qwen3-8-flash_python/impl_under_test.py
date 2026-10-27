def build_tower(n):
    width = 2 * n - 1
    return ["*" * (2 * i - 1) for i in range(1, n + 1)]

def buildTower(n):
    return build_tower(n)