import os
import stat

def create_executable_script(filename="script.sh", content="#!/bin/bash\necho \"Hello from script.sh\"\n"):
    with open(filename, "w") as f:
        f.write(content)
    current_mode = os.stat(filename).st_mode
    os.chmod(filename, current_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
    return os.path.abspath(filename)

if __name__ == "__main__":
    path = create_executable_script()
    print(f"Created executable file: {path}")