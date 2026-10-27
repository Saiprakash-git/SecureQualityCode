import os
import stat


def create_executable_script(filename: str = "script.sh", content: str = "#!/bin/sh\necho 'Hello, World!'\n") -> str:
    with open(filename, "w", encoding="utf-8") as file:
        file.write(content)

    current_mode = os.stat(filename).st_mode
    new_mode = current_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH
    os.chmod(filename, new_mode)

    return filename


if __name__ == "__main__":
    create_executable_script()