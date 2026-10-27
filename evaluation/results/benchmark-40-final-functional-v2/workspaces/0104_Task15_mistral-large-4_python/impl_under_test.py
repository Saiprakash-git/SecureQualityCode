def build_tower(floors):
    width = 2 * floors - 1
    tower = []
    for i in range(floors):
        stars = 2 * i + 1
        spaces = (width - stars) // 2
        tower.append(" " * spaces + "*" * stars + " " * spaces)
    return tower