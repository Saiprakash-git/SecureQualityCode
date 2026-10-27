import getpass
import os
import sys


PASSWORD_FILE = "password.txt"


def read_stored_password(file_path):
    with open(file_path, "r", encoding="utf-8") as file:
        return file.readline().rstrip("\n")


def compare_passwords(stored_password, entered_password):
    if len(stored_password) != len(entered_password):
        return False

    for index in range(len(stored_password)):
        if stored_password[index] != entered_password[index]:
            return False

    return True


def verify_password(file_path, entered_password):
    if not os.path.isfile(file_path):
        raise FileNotFoundError(f"Password file not found: {file_path}")

    stored_password = read_stored_password(file_path)
    return compare_passwords(stored_password, entered_password)


def main():
    try:
        entered_password = getpass.getpass("Enter your password: ")
        if verify_password(PASSWORD_FILE, entered_password):
            print("Password matches.")
            return 0
        else:
            print("Password does not match.")
            return 1
    except FileNotFoundError as error:
        print(error)
        return 2
    except OSError as error:
        print(f"Error reading password file: {error}")
        return 3


if __name__ == "__main__":
    sys.exit(main())