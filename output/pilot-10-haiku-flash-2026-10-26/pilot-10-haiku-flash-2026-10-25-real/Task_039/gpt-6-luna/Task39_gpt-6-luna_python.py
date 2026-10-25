import subprocess


def display_file(filename):
    return subprocess.run(["cat", "--", filename], check=False).returncode


def main():
    filename = input("Filename: ")
    return display_file(filename)


if __name__ == "__main__":
    main()