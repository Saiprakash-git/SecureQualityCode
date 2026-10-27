import html
import re
import unicodedata

MAX_USERNAME_LENGTH = 20
MAX_AGE_LENGTH = 3
MAX_EMAIL_LENGTH = 254
MIN_AGE = 0
MAX_AGE = 120

USERNAME_PATTERN = re.compile(r"\A[A-Za-z0-9_]{3,20}\Z")
AGE_PATTERN = re.compile(r"\A\d{1,3}\Z")
EMAIL_PATTERN = re.compile(
    r"\A[A-Za-z0-9._%+-]{1,64}@"
    r"[A-Za-z0-9.-]{1,255}\.[A-Za-z]{2,63}\Z"
)


def normalize_user_input(value, max_length):
    if not isinstance(value, str):
        raise TypeError("Input must be a string.")

    normalized = unicodedata.normalize("NFC", value).strip()

    if len(normalized) > max_length:
        raise ValueError(f"Input is longer than {max_length} characters.")

    if any(ord(character) < 32 or ord(character) == 127 for character in normalized):
        raise ValueError("Input contains control characters.")

    return normalized


def validate_username(raw_username):
    username = normalize_user_input(raw_username, MAX_USERNAME_LENGTH)

    if not USERNAME_PATTERN.match(username):
        raise ValueError(
            "Username must contain only letters, numbers, and underscores, "
            "and must be 3 to 20 characters long."
        )

    return username


def validate_age(raw_age):
    age_text = normalize_user_input(raw_age, MAX_AGE_LENGTH)

    if not AGE_PATTERN.match(age_text):
        raise ValueError("Age must contain digits only.")

    age = int(age_text)

    if age < MIN_AGE or age > MAX_AGE:
        raise ValueError(f"Age must be between {MIN_AGE} and {MAX_AGE}.")

    return age


def validate_email(raw_email):
    email = normalize_user_input(raw_email, MAX_EMAIL_LENGTH)

    if not EMAIL_PATTERN.match(email):
        raise ValueError("Email address is not in an accepted format.")

    return email


def sanitize_for_display(value):
    return html.escape(str(value), quote=True)


def read_validated_input(prompt, validator):
    try:
        raw_input_value = input(prompt)
    except (EOFError, KeyboardInterrupt):
        return None

    try:
        return validator(raw_input_value)
    except ValueError as error:
        print(f"Input rejected: {error}")
        return None


def build_profile(username, age, email):
    return {
        "username": username,
        "age": age,
        "email": email,
        "age_next_year": age + 1,
    }


def main():
    username = read_validated_input(
        "Enter username (3-20 letters, numbers, or underscores): ",
        validate_username,
    )
    if username is None:
        return

    age = read_validated_input("Enter age (0-120): ", validate_age)
    if age is None:
        return

    email = read_validated_input("Enter email address: ", validate_email)
    if email is None:
        return

    profile = build_profile(username, age, email)

    print("Validated profile summary:")
    print(f"Username: {sanitize_for_display(profile['username'])}")
    print(f"Age: {profile['age']}")
    print(f"Age next year: {profile['age_next_year']}")
    print(f"Email: {sanitize_for_display(profile['email'])}")


if __name__ == "__main__":
    main()