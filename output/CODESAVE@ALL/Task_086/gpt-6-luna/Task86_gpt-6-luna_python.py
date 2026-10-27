import argparse
import hashlib
import os
import re
import subprocess
import sys
import tempfile
import urllib.parse
import urllib.request


MAX_DOWNLOAD_BYTES = 512 * 1024 * 1024


class HTTPSOnlyRedirectHandler(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, request, response, code, message, headers, new_url):
        parsed = urllib.parse.urlparse(new_url)
        if parsed.scheme.lower() != "https":
            raise ValueError("Refusing to follow a redirect that is not HTTPS.")
        return super().redirect_request(
            request, response, code, message, headers, new_url
        )


def download_and_execute(url, expected_sha256, executable_args=None):
    """Download an HTTPS EXE, verify its SHA-256 digest, and run it on Windows."""
    if os.name != "nt":
        raise RuntimeError("This program can execute EXE files only on Windows.")

    parsed = urllib.parse.urlparse(url)
    if parsed.scheme.lower() != "https" or not parsed.hostname:
        raise ValueError("The URL must use HTTPS and include a hostname.")
    if parsed.username or parsed.password:
        raise ValueError("URLs containing embedded credentials are not allowed.")

    expected_sha256 = expected_sha256.strip().lower()
    if not re.fullmatch(r"[0-9a-f]{64}", expected_sha256):
        raise ValueError("expected_sha256 must contain exactly 64 hexadecimal characters.")

    request = urllib.request.Request(
        url,
        headers={"User-Agent": "Verified-EXE-Launcher/1.0"},
    )
    opener = urllib.request.build_opener(HTTPSOnlyRedirectHandler())
    temporary_path = None

    try:
        with opener.open(request, timeout=30) as response:
            final_url = urllib.parse.urlparse(response.geturl())
            if final_url.scheme.lower() != "https":
                raise ValueError("The final download URL is not HTTPS.")

            content_length = response.headers.get("Content-Length")
            if content_length is not None:
                try:
                    if int(content_length) > MAX_DOWNLOAD_BYTES:
                        raise ValueError("The download exceeds the configured size limit.")
                except ValueError as exc:
                    if str(exc).startswith("The download"):
                        raise
                    raise ValueError("The server returned an invalid Content-Length.") from exc

            digest = hashlib.sha256()
            total_size = 0

            with tempfile.NamedTemporaryFile(
                mode="wb", suffix=".exe", prefix="verified-download-", delete=False
            ) as output:
                temporary_path = output.name
                while True:
                    chunk = response.read(1024 * 1024)
                    if not chunk:
                        break
                    total_size += len(chunk)
                    if total_size > MAX_DOWNLOAD_BYTES:
                        raise ValueError("The download exceeds the configured size limit.")
                    digest.update(chunk)
                    output.write(chunk)

        if total_size == 0:
            raise ValueError("The downloaded file is empty.")
        if digest.hexdigest() != expected_sha256:
            raise ValueError("SHA-256 verification failed; the file will not be executed.")

        completed = subprocess.run(
            [temporary_path, *(executable_args or [])],
            shell=False,
            check=False,
        )
        return completed.returncode
    finally:
        if temporary_path and os.path.exists(temporary_path):
            try:
                os.remove(temporary_path)
            except OSError:
                pass


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Download an HTTPS EXE, verify its SHA-256 digest, and run it."
    )
    parser.add_argument("url", help="HTTPS URL of the EXE file")
    parser.add_argument(
        "--sha256",
        required=True,
        help="Expected SHA-256 digest of the EXE (64 hexadecimal characters)",
    )
    parser.add_argument(
        "--arg",
        action="append",
        default=[],
        dest="executable_args",
        help="Argument to pass to the EXE; may be specified more than once",
    )
    options = parser.parse_args(argv)

    try:
        confirmation = input(
            "This will execute the downloaded program after digest verification. "
            "Continue? Type 'yes' to proceed: "
        )
        if confirmation.strip().lower() != "yes":
            return 1
        return download_and_execute(
            options.url,
            options.sha256,
            options.executable_args,
        )
    except (OSError, ValueError, RuntimeError, urllib.error.URLError) as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())