#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <unistd.h>
#include <termios.h>

#define NAME_SIZE 256
#define EMAIL_SIZE 320
#define PASSWORD_SIZE 1024
#define SALT_SIZE 16
#define HASH_SIZE 32
#define PBKDF2_ITERATIONS 200000

static int read_line(const char *prompt, char *buffer, size_t size, int hide_input)
{
    struct termios original;
    int terminal = hide_input && isatty(STDIN_FILENO);
    int changed = 0;

    if (prompt != NULL) {
        fputs(prompt, stdout);
        fflush(stdout);
    }

    if (terminal && tcgetattr(STDIN_FILENO, &original) == 0) {
        struct termios hidden = original;
        hidden.c_lflag &= (tcflag_t)~ECHO;
        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &hidden) == 0)
            changed = 1;
    }

    if (fgets(buffer, (int)size, stdin) == NULL) {
        if (changed) {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
            putchar('\n');
        }
        return 0;
    }

    if (changed) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
        putchar('\n');
    }

    size_t length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[--length] = '\0';
        if (length > 0 && buffer[length - 1] == '\r')
            buffer[--length] = '\0';
        return 1;
    }

    if (length == size - 1) {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF)
            ;
        return -1;
    }

    return 1;
}

static void trim_whitespace(char *text)
{
    char *start = text;
    while (*start != '\0' && isspace((unsigned char)*start))
        start++;

    if (start != text)
        memmove(text, start, strlen(start) + 1);

    size_t length = strlen(text);
    while (length > 0 && isspace((unsigned char)text[length - 1]))
        text[--length] = '\0';
}

static int valid_email(const char *email)
{
    const char *at = strchr(email, '@');
    if (at == NULL || at == email || strchr(at + 1, '@') != NULL)
        return 0;

    const char *dot = strchr(at + 1, '.');
    return dot != NULL && dot != at + 1 && dot[1] != '\0';
}

static int create_database(sqlite3 *database)
{
    const char *sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT NOT NULL,"
        "email TEXT NOT NULL COLLATE NOCASE UNIQUE,"
        "password_salt BLOB NOT NULL,"
        "password_hash BLOB NOT NULL,"
        "password_iterations INTEGER NOT NULL,"
        "created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ");";

    char *error = NULL;
    int result = sqlite3_exec(database, sql, NULL, NULL, &error);
    if (result != SQLITE_OK) {
        fprintf(stderr, "Could not initialize database: %s\n",
                error != NULL ? error : sqlite3_errmsg(database));
        sqlite3_free(error);
        return 0;
    }
    return 1;
}

static int register_user(sqlite3 *database, const char *name, const char *email,
                         const char *password)
{
    unsigned char salt[SALT_SIZE];
    unsigned char hash[HASH_SIZE];

    if (RAND_bytes(salt, sizeof(salt)) != 1) {
        fprintf(stderr, "Could not generate a secure password salt.\n");
        return 0;
    }

    if (PKCS5_PBKDF2_HMAC(password, (int)strlen(password), salt, sizeof(salt),
                          PBKDF2_ITERATIONS, EVP_sha256(), sizeof(hash), hash) != 1) {
        fprintf(stderr, "Could not securely process the password.\n");
        OPENSSL_cleanse(salt, sizeof(salt));
        return 0;
    }

    const char *sql =
        "INSERT INTO users (name, email, password_salt, password_hash, password_iterations) "
        "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt *statement = NULL;
    int result = sqlite3_prepare_v2(database, sql, -1, &statement, NULL);
    if (result == SQLITE_OK)
        result = sqlite3_bind_text(statement, 1, name, -1, SQLITE_TRANSIENT);
    if (result == SQLITE_OK)
        result = sqlite3_bind_text(statement, 2, email, -1, SQLITE_TRANSIENT);
    if (result == SQLITE_OK)
        result = sqlite3_bind_blob(statement, 3, salt, sizeof(salt), SQLITE_TRANSIENT);
    if (result == SQLITE_OK)
        result = sqlite3_bind_blob(statement, 4, hash, sizeof(hash), SQLITE_TRANSIENT);
    if (result == SQLITE_OK)
        result = sqlite3_bind_int(statement, 5, PBKDF2_ITERATIONS);
    if (result == SQLITE_OK)
        result = sqlite3_step(statement);

    if (result != SQLITE_DONE) {
        if (result == SQLITE_CONSTRAINT || sqlite3_extended_errcode(database) == SQLITE_CONSTRAINT_UNIQUE)
            fprintf(stderr, "An account with that email address already exists.\n");
        else
            fprintf(stderr, "Could not register user: %s\n", sqlite3_errmsg(database));
        sqlite3_finalize(statement);
        OPENSSL_cleanse(salt, sizeof(salt));
        OPENSSL_cleanse(hash, sizeof(hash));
        return 0;
    }

    sqlite3_finalize(statement);
    OPENSSL_cleanse(salt, sizeof(salt));
    OPENSSL_cleanse(hash, sizeof(hash));
    return 1;
}

int main(void)
{
    sqlite3 *database = NULL;
    char name[NAME_SIZE];
    char email[EMAIL_SIZE];
    char password[PASSWORD_SIZE];

    int result = sqlite3_open("registration.db", &database);
    if (result != SQLITE_OK) {
        fprintf(stderr, "Could not open database: %s\n",
                database != NULL ? sqlite3_errmsg(database) : "unknown error");
        sqlite3_close(database);
        return EXIT_FAILURE;
    }

    if (!create_database(database)) {
        sqlite3_close(database);
        return EXIT_FAILURE;
    }

    int input_result = read_line("Name: ", name, sizeof(name), 0);
    if (input_result != 1) {
        fprintf(stderr, "Invalid or missing name input.\n");
        sqlite3_close(database);
        return EXIT_FAILURE;
    }

    input_result = read_line("Email: ", email, sizeof(email), 0);
    if (input_result != 1) {
        fprintf(stderr, "Invalid or missing email input.\n");
        sqlite3_close(database);
        return EXIT_FAILURE;
    }

    input_result = read_line("Password: ", password, sizeof(password), 1);
    if (input_result != 1) {
        fprintf(stderr, "Invalid or missing password input.\n");
        OPENSSL_cleanse(password, sizeof(password));
        sqlite3_close(database);
        return EXIT_FAILURE;
    }

    trim_whitespace(name);
    trim_whitespace(email);

    if (name[0] == '\0' || !valid_email(email) || password[0] == '\0') {
        fprintf(stderr, "Please provide a name, a valid email address, and a password.\n");
        OPENSSL_cleanse(password, sizeof(password));
        sqlite3_close(database);
        return EXIT_FAILURE;
    }

    int registered = register_user(database, name, email, password);
    OPENSSL_cleanse(password, sizeof(password));
    sqlite3_close(database);

    if (!registered)
        return EXIT_FAILURE;

    printf("Registration successful.\n");
    return EXIT_SUCCESS;
}