import os
import stat


def create_executable_script(path="script.sh", content="#!/bin/sh\necho 'Hello, world!'\n"):
    with open(path, "w", encoding="utf-8") as file:
        file.write(content)

    current_mode = os.stat(path).st_mode
    executable_mode = current_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH
    os.chmod(path, executable_mode)

    return path


if __name__ == "__main__":
    create_executable_script()