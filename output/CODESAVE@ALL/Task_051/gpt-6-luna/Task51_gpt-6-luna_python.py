import argparse
import string


def encrypt(text: str, shift: int = 3) -> str:
    """Encrypt text with a Caesar cipher, preserving letter case and punctuation."""
    shift %= 26
    result = []

    for character in text:
        if character in string.ascii_lowercase:
            result.append(chr((ord(character) - ord("a") + shift) % 26 + ord("a")))
        elif character in string.ascii_uppercase:
            result.append(chr((ord(character) - ord("A") + shift) % 26 + ord("A")))
        else:
            result.append(character)

    return "".join(result)


def main() -> None:
    parser = argparse.ArgumentParser(description="Encrypt text with a Caesar cipher.")
    parser.add_argument("text", help="Text to encrypt.")
    parser.add_argument("--shift", type=int, default=3, help="Shift amount (default: 3).")
    arguments = parser.parse_args()
    print(encrypt(arguments.text, arguments.shift))


if __name__ == "__main__":
    main()