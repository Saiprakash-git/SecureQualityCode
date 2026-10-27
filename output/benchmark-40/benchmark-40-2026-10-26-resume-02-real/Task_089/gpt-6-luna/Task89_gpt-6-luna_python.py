import getpass
import sys


def read_stored_password(file_path):
    with open(file_path, "r", encoding="utf-8") as password_file:
        return password_file.read().rstrip("\r\n")


def passwords_match(stored_password, entered_password):
    if len(stored_password) != len(entered_password):
        return False

    matches = True
    for index in range(len(stored_password)):
        if stored_password[index] != entered_password[index]:
            matches = False

    return matches


def main(file_path):
    try:
        stored_password = read_stored_password(file_path)
    except OSError:
        print("Unable to read the password file.")
        return

    entered_password = getpass.getpass("Enter password: ")
    if passwords_match(stored_password, entered_password):
        print("Password matches.")
    else:
        print("Password does not match.")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "password.txt")