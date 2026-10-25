import os
import shlex
import subprocess

filename = input("Enter filename: ")

if os.name == "nt":
    command = f"type {subprocess.list2cmdline([filename])}"
else:
    command = f"cat {shlex.quote(filename)}"

os.system(command)