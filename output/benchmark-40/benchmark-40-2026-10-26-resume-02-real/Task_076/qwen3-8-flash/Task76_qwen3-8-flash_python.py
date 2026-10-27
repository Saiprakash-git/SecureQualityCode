import sys
import html
import urllib.request
from html.parser import HTMLParser


class TitleParser(HTMLParser):
    def __init__(self):
        super().__init__()
        self.in_title = False
        self.title_parts = []

    def handle_starttag(self, tag, attrs):
        if tag == "title":
            self.in_title = True

    def handle_data(self, data):
        if self.in_title:
            self.title_parts.append(data)

    def handle_endtag(self, tag):
        if tag == "title":
            self.in_title = False


def extract_page_title(url):
    request = urllib.request.Request(
        url,
        headers={"User-Agent": "Mozilla/5.0"}
    )

    with urllib.request.urlopen(request, timeout=30) as response:
        charset = response.headers.get_content_charset() or "utf-8"
        content = response.read()

    text = content.decode(charset, errors="replace")

    parser = TitleParser()
    parser.feed(text)

    title = "".join(parser.title_parts).strip()
    return html.unescape(title)


def main():
    if len(sys.argv) < 2:
        return

    url = sys.argv[1]

    try:
        title = extract_page_title(url)
    except Exception:
        title = ""

    print(title)


if __name__ == "__main__":
    main()