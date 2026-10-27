import os
import sys
import tempfile
import subprocess
import urllib.request
import urllib.parse
from urllib.error import URLError, HTTPError


def validate_url(url):
    if not isinstance(url, str) or not url.strip():
        return False
    parsed = urllib.parse.urlparse(url.strip())
    return parsed.scheme in ("http", "https") and bool(parsed.netloc)


def derive_filename(url, default_name="downloaded_program.exe"):
    parsed = urllib.parse.urlparse(url)
    name = os.path.basename(parsed.path)
    if not name or not name.lower().endswith(".exe"):
        return default_name
    return name


def is_pe_file(path):
    try:
        with open(path, "rb") as f:
            return f.read(2) == b"MZ"
    except OSError:
        return False


def download_exe(url, dest_dir=None, timeout=30, chunk_size=65536):
    if not validate_url(url):
        raise ValueError("Invalid URL: must be http or https with a host.")

    if dest_dir is None:
        dest_dir = tempfile.mkdtemp(prefix="exe_download_")
    os.makedirs(dest_dir, exist_ok=True)

    filename = derive_filename(url)
    dest_path = os.path.join(dest_dir, filename)

    request = urllib.request.Request(url, headers={"User-Agent": "exe-downloader/1.0"})
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response, open(dest_path, "wb") as out:
            while True:
                chunk = response.read(chunk_size)
                if not chunk:
                    break
                out.write(chunk)
    except (HTTPError, URLError, OSError) as exc:
        if os.path.exists(dest_path):
            os.remove(dest_path)
        raise RuntimeError(f"Download failed: {exc}") from exc

    if not is_pe_file(dest_path):
        os.remove(dest_path)
        raise ValueError("Downloaded file is not a valid Windows executable (missing MZ header).")

    return dest_path


def execute_exe(path, args=None, timeout=None):
    if not os.path.isfile(path):
        raise FileNotFoundError(f"Executable not found: {path}")
    command = [path] + list(args or [])
    result = subprocess.run(command, timeout=timeout)
    return result.returncode


def confirm(prompt, default=False):
    suffix = " [y/N]: " if not default else " [Y/n]: "
    try:
        answer = input(prompt + suffix).strip().lower()
    except EOFError:
        return False
    if not answer:
        return default
    return answer in ("y", "yes")


def main(argv=None):
    argv = sys.argv[1:] if argv is None else argv

    auto_yes = "--yes" in argv
    argv = [a for a in argv if a != "--yes"]

    if not argv:
        print("Usage: python exe_runner.py <URL> [--yes]")
        return 1

    url = argv[0]
    exe_args = argv[1:]

    if not validate_url(url):
        print("Error: invalid URL. Use an http:// or https:// address.")
        return 1

    if not auto_yes:
        if not confirm(f"Download and execute EXE from {url}?"):
            print("Aborted.")
            return 0

    try:
        print("Downloading...")
        path = download_exe(url)
        print(f"Saved to: {path}")
    except (ValueError, RuntimeError) as exc:
        print(f"Error: {exc}")
        return 1

    try:
        print("Executing...")
        code = execute_exe(path, args=exe_args)
        print(f"Process exited with code {code}")
        return 0
    except (OSError, FileNotFoundError, subprocess.SubprocessError) as exc:
        print(f"Error executing program: {exc}")
        return 1


if __name__ == "__main__":
    sys.exit(main())