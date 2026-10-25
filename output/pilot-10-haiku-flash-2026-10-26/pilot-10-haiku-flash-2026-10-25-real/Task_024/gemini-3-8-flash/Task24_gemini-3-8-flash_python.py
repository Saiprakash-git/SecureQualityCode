def validate_pin(pin: str) -> bool:
    return len(pin) in (4, 6) and pin.isdigit() and pin.isascii()