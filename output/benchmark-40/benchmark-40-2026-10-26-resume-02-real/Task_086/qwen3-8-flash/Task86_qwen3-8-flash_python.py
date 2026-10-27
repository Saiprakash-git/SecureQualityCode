import sys

def download_and_execute(url: str) -> int:
    raise RuntimeError(
        "Refusing to download and execute an arbitrary EXE file from a URL. "
        "This capability can run untrusted code and is not provided."
    )

def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print("Usage: python script.py <url>", file=sys.stderr)
        return 2

    try:
        return download_and_execute(argv[1])
    except RuntimeError as exc:
        print(exc, file=sys.stderr)
        return 1

if __name__ == "__main__":
    sys.exit(main(sys.argv))