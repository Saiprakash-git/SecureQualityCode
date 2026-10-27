from pathlib import Path

script = Path("script.sh")
script.write_text("#!/bin/sh\n")
script.chmod(0o755)