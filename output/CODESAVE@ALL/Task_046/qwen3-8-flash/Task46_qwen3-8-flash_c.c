#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_INPUT 256
#define DB_PATH "users.db"

static void trim_newline(char *s)
{
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n')
        s[len - 1] = '\0';
}

static void read_line(char *buf, size_t size)
{
    if (fgets(buf, (int)size, stdin) == NULL) {
        buf[0] = '\0';
        return;
    }

    trim_newline(buf);

    if (strlen(buf) == size - 1) {
        int c;
        while ((c = fgetc(stdin)) != '\n' && c != EOF)
            ;
    }
}

static int contains_forbidden(const char *s)
{
    return strchr(s, ',') != NULL;
}

static int is_valid_name(const char *name)
{
    size_t len = strlen(name);
    int letters = 0;

    if (len == 0 || len > 128)
        return 0;

    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)name[i];

        if (isalpha(c)) {
            letters++;
        } else if (c != ' ' && c != '-' && c != '\'') {
            return 0;
        }
    }

    return letters > 0;
}

static int is_valid_email(const char *email)
{
    const char *at = strchr(email, '@');
    const char *domain;
    const char *dot;

    if (at == NULL || at == email)
        return 0;

    domain = at + 1;

    if (*domain == '\0')
        return 0;

    if (strchr(domain, '@') != NULL)
        return 0;

    dot = strchr(domain, '.');
    if (dot == NULL || dot == domain || *(dot + 1) == '\0')
        return 0;

    for (size_t i = 0; email[i] != '\0'; ++i) {
        if (email[i] == ' ' || email[i] == '\t' || email[i] == ',')
            return 0;
    }

    return 1;
}

static int is_valid_password(const char *password)
{
    return strlen(password) >= 8;
}

static unsigned long long hash_password(const char *password)
{
    unsigned long long hash = 1469598103934665603ULL;

    while (*password != '\0') {
        hash ^= (unsigned char)*password;
        hash *= 1099511628211ULL;
        password++;
    }

    return hash;
}

static int case_insensitive_equal(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        int ca = tolower((unsigned char)*a);
        int cb = tolower((unsigned char)*b);

        if (ca != cb)
            return 0;

        a++;
        b++;
    }

    return *a == *b;
}

static int user_exists(const char *email, const char *db_path)
{
    FILE *db = fopen(db_path, "r");
    char line[1024];

    if (db == NULL)
        return 0;

    while (fgets(line, sizeof(line), db) != NULL) {
        char *comma = strchr(line, ',');
        char stored_email[MAX_INPUT];
        size_t email_len;

        if (comma == NULL)
            continue;

        email_len = (size_t)(comma - line);

        if (email_len >= sizeof(stored_email))
            continue;

        memcpy(stored_email, line, email_len);
        stored_email[email_len] = '\0';

        if (case_insensitive_equal(stored_email, email)) {
            fclose(db);
            return 1;
        }
    }

    fclose(db);
    return 0;
}

static int register_user(const char *name, const char *email, const char *password, const char *db_path)
{
    FILE *db;
    unsigned long long password_hash;

    if (!is_valid_name(name) || !is_valid_email(email) || !is_valid_password(password))
        return -1;

    if (contains_forbidden(name) || contains_forbidden(email))
        return -1;

    if (user_exists(email, db_path))
        return -2;

    db = fopen(db_path, "a");
    if (db == NULL)
        return -3;

    password_hash = hash_password(password);

    fprintf(db, "%s,%s,%016llx\n", name, email, password_hash);

    fclose(db);
    return 0;
}

int main(void)
{
    char name[MAX_INPUT];
    char email[MAX_INPUT];
    char password[MAX_INPUT];
    int result;

    printf("User Registration\n");
    printf("Name: ");
    read_line(name, sizeof(name));

    printf("Email: ");
    read_line(email, sizeof(email));

    printf("Password: ");
    read_line(password, sizeof(password));

    result = register_user(name, email, password, DB_PATH);

    if (result == 0) {
        printf("Registration successful.\n");
    } else if (result == -1) {
        printf("Invalid input. Name, email, and password of at least 8 characters are required.\n");
    } else if (result == -2) {
        printf("Email is already registered.\n");
    } else {
        printf("Could not store registration data.\n");
    }

    return 0;
}