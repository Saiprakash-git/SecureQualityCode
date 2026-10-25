def validate_pin(pin):
    return len(pin) in (4, 6) and all("0" <= char <= "9" for char in pin)