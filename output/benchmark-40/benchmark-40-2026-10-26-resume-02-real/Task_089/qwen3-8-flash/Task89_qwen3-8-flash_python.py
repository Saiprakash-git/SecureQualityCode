import sys

def read_password_from_file(file_path):
    try:
        with open(file_path, "r", encoding="utf-8") as file:
            return file.readline().rstrip("\r\n")
    except OSError:
        return None

def compare_passwords(stored_password, entered_password):
    if stored_password is None or entered_password is None:
        return False

    if len(stored_password) != len(entered_password):
        return False

    for index in range(len(stored_password)):
        if stored_password[index] != entered_password[index]:
            return False

    return True

def verify_password(file_path, entered_password):
    stored_password = read_password_from_file(file_path)
    return compare_passwords(stored_password, entered_password)

def main(file_path, entered_password):
    return verify_password(file_path, entered_password)

if __name__ == "__main__":
    if len(sys.argv) == 3:
        result = main(sys.argv[1], sys.argv[2])
    else:
        result = main(input("Password file path: "), input("Enter password: "))

    print("Password matches." if result else "Password does not match.")