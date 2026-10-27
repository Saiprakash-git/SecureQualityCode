import os

def create_executable_script(filename="script.sh", content="#!/bin/bash\necho 'Hello, World!'\n", permissions=0o755):
    with open(filename, 'w') as f:
        f.write(content)
    os.chmod(filename, permissions)
    return filename

if __name__ == "__main__":
    create_executable_script()