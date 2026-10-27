import math
import re
import sys
import unicodedata
from typing import Dict, List, Optional, Tuple, Union

MAX_INPUT_LENGTH = 100
MAX_TEXT_LENGTH = 500
MAX_POWER_EXPONENT = 100


class ValidationError(Exception):
    pass


def sanitize_string(raw_str: str, max_length: int = MAX_INPUT_LENGTH) -> str:
    if not isinstance(raw_str, str):
        raise ValidationError("Input must be a string.")
    if len(raw_str) > max_length:
        raise ValidationError(f"Input exceeds maximum allowed length of {max_length} characters.")
    
    normalized = unicodedata.normalize("NFKC", raw_str)
    cleaned = "".join(ch for ch in normalized if ch.isprintable() and ch not in "\r\n\t")
    return cleaned.strip()


def validate_username(raw_username: str) -> str:
    cleaned = sanitize_string(raw_username, max_length=32)
    if not re.fullmatch(r"^[a-zA-Z0-9_-]{3,32}$", cleaned):
        raise ValidationError("Username must be 3-32 characters and contain only letters, numbers, hyphens, or underscores.")
    return cleaned


def validate_integer(raw_val: str, min_val: int = -1_000_000, max_val: int = 1_000_000) -> int:
    cleaned = sanitize_string(raw_val, max_length=15)
    if not re.fullmatch(r"^-?\d+$", cleaned):
        raise ValidationError("Input is not a valid integer format.")
    
    try:
        val = int(cleaned)
    except ValueError:
        raise ValidationError("Failed to parse integer.")
    
    if val < min_val or val > max_val:
        raise ValidationError(f"Integer out of bounds: must be between {min_val} and {max_val}.")
    return val


def validate_float(raw_val: str, min_val: float = -1e9, max_val: float = 1e9) -> float:
    cleaned = sanitize_string(raw_val, max_length=20)
    if not re.fullmatch(r"^-?\d+(\.\d+)?$", cleaned):
        raise ValidationError("Input is not a valid floating-point number format.")
    
    try:
        val = float(cleaned)
    except ValueError:
        raise ValidationError("Failed to parse float.")
        
    if math.isnan(val) or math.isinf(val):
        raise ValidationError("Special numeric values (NaN, Infinity) are prohibited.")
    if val < min_val or val > max_val:
        raise ValidationError(f"Value must be between {min_val} and {max_val}.")
    return val


def validate_text(raw_text: str, min_length: int = 1, max_length: int = MAX_TEXT_LENGTH) -> str:
    cleaned = sanitize_string(raw_text, max_length=max_length)
    if len(cleaned) < min_length:
        raise ValidationError(f"Text cannot be shorter than {min_length} characters.")
    return cleaned


def tokenize_expression(expr: str) -> List[str]:
    cleaned = sanitize_string(expr, max_length=50)
    token_pattern = re.compile(r"\s*(\d+(?:\.\d+)?|[+\-*/^()]|.)")
    tokens: List[str] = []
    
    pos = 0
    while pos < len(cleaned):
        match = token_pattern.match(cleaned, pos)
        if not match:
            break
        token = match.group(1).strip()
        if token:
            if not (re.fullmatch(r"^\d+(?:\.\d+)?$", token) or token in "+-*/^()"):
                raise ValidationError(f"Forbidden character or token in expression: '{token}'")
            tokens.append(token)
        pos = match.end()
    
    if not tokens:
        raise ValidationError("Empty expression.")
    return tokens


def safe_evaluate_expression(tokens: List[str]) -> float:
    precedence = {"+": 1, "-": 1, "*": 2, "/": 2, "^": 3}
    associativity = {"+": "L", "-": "L", "*": "L", "/": "L", "^": "R"}
    output_queue: List[Union[float, str]] = []
    operator_stack: List[str] = []

    for token in tokens:
        if re.fullmatch(r"^\d+(?:\.\d+)?$", token):
            output_queue.append(float(token))
        elif token in precedence:
            while (operator_stack and operator_stack[-1] != "(" and
                   ((associativity[token] == "L" and precedence[token] <= precedence[operator_stack[-1]]) or
                    (associativity[token] == "R" and precedence[token] < precedence[operator_stack[-1]]))):
                output_queue.append(operator_stack.pop())
            operator_stack.append(token)
        elif token == "(":
            operator_stack.append(token)
        elif token == ")":
            while operator_stack and operator_stack[-1] != "(":
                output_queue.append(operator_stack.pop())
            if not operator_stack or operator_stack[-1] != "(":
                raise ValidationError("Mismatched parentheses in expression.")
            operator_stack.pop()

    while operator_stack:
        op = operator_stack.pop()
        if op in "()":
            raise ValidationError("Mismatched parentheses in expression.")
        output_queue.append(op)

    eval_stack: List[float] = []
    for item in output_queue:
        if isinstance(item, float):
            eval_stack.append(item)
        else:
            if len(eval_stack) < 2:
                raise ValidationError("Malformed arithmetic expression.")
            b = eval_stack.pop()
            a = eval_stack.pop()
            
            if item == "+":
                eval_stack.append(a + b)
            elif item == "-":
                eval_stack.append(a - b)
            elif item == "*":
                eval_stack.append(a * b)
            elif item == "/":
                if abs(b) < 1e-12:
                    raise ValidationError("Division by zero error.")
                eval_stack.append(a / b)
            elif item == "^":
                if b < 0 or b > MAX_POWER_EXPONENT or not b.is_integer():
                    raise ValidationError(f"Exponent must be an integer between 0 and {MAX_POWER_EXPONENT}.")
                eval_stack.append(math.pow(a, b))

    if len(eval_stack) != 1:
        raise ValidationError("Evaluation error: malformed expression structure.")

    result = eval_stack[0]
    if math.isnan(result) or math.isinf(result):
        raise ValidationError("Expression resulted in an undefined or out-of-bounds numeric value.")
    return result


def analyze_text(text: str) -> Dict[str, Union[int, Dict[str, int]]]:
    words = [w.lower() for w in re.findall(r"\b[a-zA-Z0-9_-]+\b", text)]
    word_freq: Dict[str, int] = {}
    for w in words:
        word_freq[w] = word_freq.get(w, 0) + 1
        
    return {
        "char_count": len(text),
        "word_count": len(words),
        "unique_words": len(word_freq),
        "frequencies": word_freq,
    }


def compute_statistics(numbers: List[float]) -> Dict[str, float]:
    if not numbers:
        raise ValidationError("Cannot compute statistics on an empty set.")
    
    n = len(numbers)
    mean_val = sum(numbers) / n
    variance = sum((x - mean_val) ** 2 for x in numbers) / n
    std_dev = math.sqrt(variance)
    
    return {
        "count": float(n),
        "sum": sum(numbers),
        "min": min(numbers),
        "max": max(numbers),
        "mean": mean_val,
        "std_dev": std_dev,
    }


def run_pipeline() -> None:
    print("=== Secure Input Processing Application ===")
    
    try:
        raw_user = input("Enter username (alphanumeric, 3-32 chars): ")
        username = validate_username(raw_user)
        print(f"Validated Username: {username}\n")
    except ValidationError as e:
        print(f"Validation Failure: {e}")
        return

    try:
        raw_expr = input("Enter safe math expression (e.g., '3 + 5 * (2 ^ 4)'): ")
        tokens = tokenize_expression(raw_expr)
        result = safe_evaluate_expression(tokens)
        print(f"Calculated Result: {result}\n")
    except ValidationError as e:
        print(f"Expression Error: {e}\n")

    try:
        raw_text = input("Enter a message to analyze: ")
        validated_text = validate_text(raw_text)
        stats = analyze_text(validated_text)
        print(f"Text Analysis Results: {stats}\n")
    except ValidationError as e:
        print(f"Text Error: {e}\n")

    try:
        raw_count = input("How many numbers do you want to input (1-10)? ")
        count = validate_integer(raw_count, min_val=1, max_val=10)
        
        numbers: List[float] = []
        for i in range(count):
            raw_num = input(f"Enter number #{i + 1} (-1000 to 1000): ")
            num = validate_float(raw_num, min_val=-1000.0, max_val=1000.0)
            numbers.append(num)
            
        num_stats = compute_statistics(numbers)
        print("\nNumeric Statistics:")
        for k, v in num_stats.items():
            print(f"  {k}: {v:.4f}")
    except ValidationError as e:
        print(f"Numeric Processing Error: {e}")


if __name__ == "__main__":
    run_pipeline()