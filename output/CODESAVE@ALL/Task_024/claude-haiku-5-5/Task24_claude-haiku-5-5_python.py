def validate_pin(pin):
    if not isinstance(pin, str):
        return False
    if len(pin) not in (4, 6):
        return False
    return all(c in "0123456789" for c in pin)