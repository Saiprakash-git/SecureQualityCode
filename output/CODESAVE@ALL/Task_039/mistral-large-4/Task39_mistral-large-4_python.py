import os
import platform

def display_file(filename):
    if platform.system() == "Windows":
        os.system(f"type {filename}")
    else:
        os.system(f"cat {filename}")

filename = input("Enter filename: ")
display_file(filename)