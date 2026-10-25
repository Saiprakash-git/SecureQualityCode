def valid_pin(pin):
    if not isinstance(pin, str):
        return False
    return len(pin) in (4, 6) and all('0' <= ch <= '9' for ch in pin)