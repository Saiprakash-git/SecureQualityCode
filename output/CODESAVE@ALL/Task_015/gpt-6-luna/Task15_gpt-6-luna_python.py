def tower_builder(n_floors):
    width = 2 * n_floors - 1
    return [
        " " * (n_floors - floor - 1)
        + "*" * (2 * floor + 1)
        + " " * (n_floors - floor - 1)
        for floor in range(n_floors)
    ]