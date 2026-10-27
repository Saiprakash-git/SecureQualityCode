#!/usr/bin/env python3
import argparse
import secrets


def generate_token(entropy_bytes: int = 32) -> str:
    """Generate a cryptographically secure URL-safe bearer token."""
    if entropy_bytes < 16:
        raise ValueError("entropy_bytes must be at least 16")
    return secrets.token_urlsafe(entropy_bytes)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Generate a secure authentication token.")
    parser.add_argument(
        "--entropy-bytes",
        type=int,
        default=32,
        help="Random bytes used to generate the token (minimum: 16; default: 32).",
    )
    args = parser.parse_args(argv)

    try:
        print(generate_token(args.entropy_bytes))
    except ValueError as error:
        parser.error(str(error))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())