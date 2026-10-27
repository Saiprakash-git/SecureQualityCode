import sys
import re
import ssl
import urllib.request
import urllib.error
from html import unescape


def fetch_html(url):
    context = ssl.create_default_context()
    request = urllib.request.Request(
        url,
        headers={"User-Agent": "Mozilla/5.0 (title-extractor/1.0)"}
    )
    with urllib.request.urlopen(request, context=context, timeout=15) as response:
        charset = response.headers.get_content_charset() or "utf-8"
        raw = response.read()
        return raw.decode(charset, errors="replace")


def extract_title(html_content):
    match = re.search(r"<title[^>]*>(.*?)</title>", html_content, re.IGNORECASE | re.DOTALL)
    if not match:
        return None
    title = unescape(match.group(1))
    return re.sub(r"\s+", " ", title).strip()


def get_page_title(url):
    if not url.lower().startswith("https://"):
        raise ValueError("URL must use HTTPS")
    html_content = fetch_html(url)
    return extract_title(html_content)


def main():
    if len(sys.argv) < 2:
        print("Usage: python script.py <https_url>", file=sys.stderr)
        sys.exit(1)
    url = sys.argv[1]
    try:
        title = get_page_title(url)
        if title is None:
            print("No title found", file=sys.stderr)
            sys.exit(2)
        print(title)
    except (urllib.error.URLError, ValueError) as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(3)


if __name__ == "__main__":
    main()