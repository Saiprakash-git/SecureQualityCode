import re
import math
import html

MAX_INPUT_LENGTH = 1000
ALLOWED_OPERATIONS = {"upper", "lower", "reverse", "word_count", "char_count", "sanitize", "is_palindrome", "evaluate_math"}
MATH_PATTERN = re.compile(r"^[0-9+\-*/().\s]+$")
TEXT_PATTERN = re.compile(r"^[\w\s.,!?'\-]+$")
MAX_MATH_LENGTH = 100


def validate_text(text):
    if not isinstance(text, str):
        raise ValueError("Input must be a string.")
    if len(text) == 0:
        raise ValueError("Input cannot be empty.")
    if len(text) > MAX_INPUT_LENGTH:
        raise ValueError(f"Input exceeds maximum length of {MAX_INPUT_LENGTH} characters.")
    if not TEXT_PATTERN.fullmatch(text):
        raise ValueError("Input contains disallowed characters.")
    return text.strip()


def validate_operation(operation):
    if not isinstance(operation, str):
        raise ValueError("Operation must be a string.")
    op = operation.strip().lower()
    if op not in ALLOWED_OPERATIONS:
        raise ValueError(f"Unsupported operation: {op}")
    return op


def validate_math_expression(expr):
    if not isinstance(expr, str):
        raise ValueError("Expression must be a string.")
    if len(expr) == 0 or len(expr) > MAX_MATH_LENGTH:
        raise ValueError("Invalid expression length.")
    if not MATH_PATTERN.fullmatch(expr):
        raise ValueError("Expression contains disallowed characters.")
    return expr.strip()


def safe_evaluate_math(expr):
    expr = validate_math_expression(expr)
    tokens = re.findall(r"\d+\.?\d*|[+\-*/()]", expr)
    if "".join(tokens).replace(" ", "") != expr.replace(" ", ""):
        raise ValueError("Malformed expression.")
    pos = 0

    def peek():
        return tokens[pos] if pos < len(tokens) else None

    def consume():
        nonlocal pos
        tok = tokens[pos]
        pos += 1
        return tok

    def parse_expression():
        value = parse_term()
        while peek() in ("+", "-"):
            op = consume()
            rhs = parse_term()
            value = value + rhs if op == "+" else value - rhs
        return value

    def parse_term():
        value = parse_factor()
        while peek() in ("*", "/"):
            op = consume()
            rhs = parse_factor()
            if op == "*":
                value = value * rhs
            else:
                if rhs == 0:
                    raise ValueError("Division by zero.")
                value = value / rhs
        return value

    def parse_factor():
        tok = peek()
        if tok == "-":
            consume()
            return -parse_factor()
        if tok == "(":
            consume()
            value = parse_expression()
            if peek() != ")":
                raise ValueError("Mismatched parentheses.")
            consume()
            return value
        if tok is not None and re.fullmatch(r"\d+\.?\d*", tok):
            consume()
            return float(tok)
        raise ValueError("Unexpected token in expression.")

    result = parse_expression()
    if pos != len(tokens):
        raise ValueError("Malformed expression.")
    if math.isnan(result) or math.isinf(result):
        raise ValueError("Result is not a finite number.")
    return result


def perform_operation(operation, text):
    op = validate_operation(operation)
    if op == "evaluate_math":
        return safe_evaluate_math(text)
    clean = validate_text(text)
    if op == "upper":
        return clean.upper()
    if op == "lower":
        return clean.lower()
    if op == "reverse":
        return clean[::-1]
    if op == "word_count":
        return len(clean.split())
    if op == "char_count":
        return len(clean)
    if op == "sanitize":
        return html.escape(clean)
    if op == "is_palindrome":
        normalized = re.sub(r"[^a-z0-9]", "", clean.lower())
        return normalized == normalized[::-1] and len(normalized) > 0
    raise ValueError("Unsupported operation.")


def run_cli():
    print("Text Processing Tool")
    print("Operations: " + ", ".join(sorted(ALLOWED_OPERATIONS)))
    while True:
        try:
            operation = input("\nEnter operation (or 'quit'): ").strip()
            if operation.lower() in ("quit", "exit"):
                break
            text = input("Enter input: ")
            result = perform_operation(operation, text)
            print("Result:", result)
        except ValueError as e:
            print("Error:", e)
        except (EOFError, KeyboardInterrupt):
            print()
            break


if __name__ == "__main__":
    run_cli()