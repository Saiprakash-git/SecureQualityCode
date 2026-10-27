import getpass
import os


def read_stored_password(filepath: str) -> str:
    """Reads the stored password from a file and strips trailing newline characters."""
    with open(filepath, "r", encoding="utf-8") as file:
        return file.read().rstrip("\r\n")


def compare_passwords_sequentially(stored_password: str, entered_password: str) -> bool:
    """
    Compares the stored password with the entered password character by character sequentially.
    Returns True if every character matches and the lengths are equal; otherwise, False.
    """
    if len(stored_password) != len(entered_password):
        return False

    for i in range(len(stored_password)):
        if stored_password[i] != entered_password[i]:
            return False

    return True


def verify_user(filepath: str, entered_password: str) -> bool:
    """Loads stored password from file and compares it to the entered password."""
    stored_password = read_stored_password(filepath)
    return compare_passwords_sequentially(stored_password, entered_password)


def main():
    password_file = "stored_password.txt"

    # Setup a sample password file if one does not exist for demonstration purposes
    if not os.path.exists(password_file):
        with open(password_file, "w", encoding="utf-8") as f:
            f.write("SecretPass123")
        print(f"Sample password file '{password_file}' created for demonstration.")

    try:
        user_input = getpass.getpass("Enter your password: ")
    except Exception:
        user_input = input("Enter your password: ")

    is_authenticated = verify_user(password_file, user_input)

    if is_authenticated:
        print("Access granted: Passwords match.")
    else:
        print("Access denied: Password mismatch.")


if __name__ == "__main__":
    main()