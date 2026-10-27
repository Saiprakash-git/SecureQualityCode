import sys
import urllib.request
import re

def get_page_title(url):
    try:
        with urllib.request.urlopen(url) as response:
            html = response.read().decode('utf-8', errors='ignore')
        match = re.search(r'<title[^>]*>(.*?)</title>', html, re.IGNORECASE | re.DOTALL)
        if match:
            title = match.group(1).strip()
            title = re.sub(r'\s+', ' ', title)
            return title
        return None
    except Exception:
        return None

if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(1)
    url = sys.argv[1]
    title = get_page_title(url)
    if title:
        print(title)