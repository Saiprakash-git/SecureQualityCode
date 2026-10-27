import re
from decimal import Decimal, DecimalException, localcontext

NUMBER_PATTERN = re.compile(r"[+-]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][+-]?\d+)?")
MAX_INPUT_LENGTH = 128
MAX_MAGNITUDE = Decimal("1e100")
MIN_MAGNITUDE = Decimal("1e-100")


def parse_number(text):
    if not isinstance(text, str):
        raise ValueError("Input must be text.")
    text = text.strip()
    if len(text) > MAX_INPUT_LENGTH or not NUMBER_PATTERN.fullmatch(text):
        raise ValueError("Enter a valid number.")
    try:
        value = Decimal(text)
    except DecimalException as exc:
        raise ValueError("Enter a valid number.") from exc

    if not value.is_finite():
        raise ValueError("Enter a finite number.")
    if value != 0 and (abs(value) > MAX_MAGNITUDE or abs(value) < MIN_MAGNITUDE):
        raise ValueError("Number is outside the supported range.")
    return value


def calculate(operation, left, right):
    if operation not in {"1", "2", "3", "4", "5", "6"}:
        raise ValueError("Choose a valid operation.")

    with localcontext() as context:
        context.prec = 50
        context.Emax = 200
        context.Emin = -200

        if operation == "1":
            result = left + right
        elif operation == "2":
            result = left - right
        elif operation == "3":
            result = left * right
        elif operation == "4":
            if right == 0:
                raise ValueError("Cannot divide by zero.")
            result = left / right
        elif operation == "5":
            if right == 0:
                raise ValueError("Cannot take a remainder with zero.")
            result = left % right
        else:
            if right != right.to_integral_value() or abs(right) > 100:
                raise ValueError("The exponent must be an integer from -100 to 100.")
            exponent = int(right)
            if left == 0 and exponent < 0:
                raise ValueError("Zero cannot be raised to a negative power.")
            result = left ** exponent

    if not result.is_finite() or abs(result) > MAX_MAGNITUDE:
        raise ValueError("The result is outside the supported range.")
    if result != 0 and abs(result) < MIN_MAGNITUDE:
        raise ValueError("The result is outside the supported range.")
    return result


def format_number(value):
    text = format(value, "f")
    if "." in text:
        text = text.rstrip("0").rstrip(".")
    return "0" if text in {"-0", ""} else text


def run():
    operations = {
        "1": "Add",
        "2": "Subtract",
        "3": "Multiply",
        "4": "Divide",
        "5": "Remainder",
        "6": "Power",
    }

    print("Calculator")
    while True:
        print("\nChoose an operation:")
        for key, name in operations.items():
            print(f"{key}. {name}")
        print("Q. Quit")

        try:
            operation = input("Operation: ").strip().lower()
            if operation == "q":
                return
            if operation not in operations:
                print("Invalid operation.")
                continue

            left = parse_number(input("First number: "))
            right = parse_number(input("Second number: "))
            result = calculate(operation, left, right)
            print("Result:", format_number(result))
        except ValueError as exc:
            print("Error:", exc)
        except (EOFError, KeyboardInterrupt):
            print()
            return
        except DecimalException:
            print("Error: Calculation is outside the supported range.")


if __name__ == "__main__":
    run()