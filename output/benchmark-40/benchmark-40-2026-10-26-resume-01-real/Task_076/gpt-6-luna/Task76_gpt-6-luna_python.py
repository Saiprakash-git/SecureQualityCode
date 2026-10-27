import sys
from html.parser import HTMLParser
from urllib.request import Request, urlopen


class _TitleParser(HTMLParser):
    def __init__(self):
        super().__init__(convert_charrefs=True)
        self._in_title = False
        self._parts = []

    def handle_starttag(self, tag, attrs):
        if tag.lower() == "title" and not self._in_title:
            self._in_title = True

    def handle_endtag(self, tag):
        if tag.lower() == "title" and self._in_title:
            self._in_title = False

    def handle_data(self, data):
        if self._in_title:
            self._parts.append(data)

    def title(self):
        return "".join(self._parts).strip()


def get_page_title(url):
    request = Request(url, headers={"User-Agent": "Mozilla/5.0"})
    with urlopen(request, timeout=30) as response:
        content = response.read()
        encoding = response.headers.get_content_charset() or "utf-8"

    parser = _TitleParser()
    parser.feed(content.decode(encoding, errors="replace"))
    parser.close()
    return parser.title()


if __name__ == "__main__":
    print(get_page_title(sys.argv[1]))