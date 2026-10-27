def build_tower(n_floors):
    width = 2 * n_floors - 1
    tower = []
    for i in range(n_floors):
        stars = 2 * i + 1
        padding = (width - stars) // 2
        tower.append(" " * padding + "*" * stars + " " * padding)
    return tower