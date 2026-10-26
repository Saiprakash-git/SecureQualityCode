int binary_array_to_int(const int *bits, int length) {
    int value = 0;

    if (bits == 0 || length <= 0) {
        return 0;
    }

    for (int i = 0; i < length; ++i) {
        value = value * 2 + (bits[i] ? 1 : 0);
    }

    return value;
}