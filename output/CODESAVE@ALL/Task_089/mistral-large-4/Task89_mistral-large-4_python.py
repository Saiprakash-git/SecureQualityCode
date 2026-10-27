def read_password_from_file(filepath):
    with open(filepath, 'r') as file:
        return file.read().strip()

def compare_characters_sequentially(stored_password, entered_password):
    if len(stored_password) != len(entered_password):
        return False
    for i in range(len(stored_password)):
        if stored_password[i] != entered_password[i]:
            return False
    return True

def verify_password(filepath, entered_password):
    stored_password = read_password_from_file(filepath)
    return compare_characters_sequentially(stored_password, entered_password)

def main():
    filepath = "password.txt"
    user_password = input("Enter password: ")
    result = verify_password(filepath, user_password)
    if result:
        print("Password correct")
    else:
        print("Password incorrect")
    return result

if __name__ == "__main__":
    main()