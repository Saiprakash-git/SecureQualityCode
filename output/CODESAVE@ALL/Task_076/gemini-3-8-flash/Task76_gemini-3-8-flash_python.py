import sys
import urllib.request
import html
import re

def get_page_title(url: str) -> str:
    req = urllib.request.Request(
        url,
        headers={"User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64)"}
    )
    with urllib.request.urlopen(req) as response:
        charset = response.headers.get_content_charset() or "utf-8"
        content = response.read().decode(charset, errors="replace")

    match = re.search(r"<title(?:\s+[^>]*)?>(.*?)</title>", content, re.IGNORECASE | re.DOTALL)
    if match:
        return html.unescape(match.group(1).strip())
    return ""

def main(args: list[str]) -> int:
    if len(args) < 2:
        print("Usage: python script.py <HTTPS_URL>", file=sys.stderr)
        return 1

    url = args[1]
    if not url.startswith("https://"):
        print("Error: URL must begin with https://", file=sys.stderr)
        return 1

    try:
        title = get_page_title(url)
        print(title)
        return 0
    except Exception as e:
        print(f"Error retrieving page title: {e}", file=sys.stderr)
        return 1

if __name__ == "__main__":
    sys.exit(main(sys.argv))