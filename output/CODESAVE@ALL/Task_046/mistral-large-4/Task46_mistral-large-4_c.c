#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#define MAX_NAME_LEN 100
#define MAX_EMAIL_LEN 100
#define MAX_PASSWORD_LEN 100
#define DB_FILE "users.db"

typedef struct {
    char name[MAX_NAME_LEN];
    char email[MAX_EMAIL_LEN];
    char password[MAX_PASSWORD_LEN];
} User;

bool validate_email(const char *email) {
    if (email == NULL || strlen(email) == 0) return false;
    const char *at = strchr(email, '@');
    if (at == NULL || at == email || at[1] == '\0') return false;
    const char *dot = strchr(at, '.');
    if (dot == NULL || dot == at + 1 || dot[1] == '\0') return false;
    return true;
}

bool validate_password(const char *password) {
    if (password == NULL || strlen(password) < 6) return false;
    return true;
}

bool validate_name(const char *name) {
    if (name == NULL || strlen(name) == 0) return false;
    for (int i = 0; name[i] != '\0'; i++) {
        if (!isalpha(name[i]) && name[i] != ' ' && name[i] != '-' && name[i] != '\'') {
            return false;
        }
    }
    return true;
}

bool register_user(const char *name, const char *email, const char *password, const char *db_filename) {
    if (!validate_name(name) || !validate_email(email) || !validate_password(password)) {
        return false;
    }

    FILE *file = fopen(db_filename, "a");
    if (file == NULL) {
        return false;
    }

    fprintf(file, "%s|%s|%s\n", name, email, password);
    fclose(file);
    return true;
}

void display_users(const char *db_filename) {
    FILE *file = fopen(db_filename, "r");
    if (file == NULL) {
        printf("No users found or database error.\n");
        return;
    }

    char line[300];
    printf("\nRegistered Users:\n");
    printf("----------------------------------------\n");
    while (fgets(line, sizeof(line), file) != NULL) {
        char name[MAX_NAME_LEN], email[MAX_EMAIL_LEN], password[MAX_PASSWORD_LEN];
        if (sscanf(line, "%[^|]|%[^|]|%[^\n]", name, email, password) == 3) {
            printf("Name: %s\nEmail: %s\nPassword: %s\n", name, email, password);
            printf("----------------------------------------\n");
        }
    }
    fclose(file);
}

int main(void) {
    char name[MAX_NAME_LEN];
    char email[MAX_EMAIL_LEN];
    char password[MAX_PASSWORD_LEN];

    printf("=== User Registration System ===\n");

    printf("Enter your name: ");
    if (fgets(name, sizeof(name), stdin) == NULL) return 1;
    name[strcspn(name, "\n")] = 0;

    printf("Enter your email: ");
    if (fgets(email, sizeof(email), stdin) == NULL) return 1;
    email[strcspn(email, "\n")] = 0;

    printf("Enter your password (min 6 chars): ");
    if (fgets(password, sizeof(password), stdin) == NULL) return 1;
    password[strcspn(password, "\n")] = 0;

    if (register_user(name, email, password, DB_FILE)) {
        printf("\nRegistration successful! User stored in database.\n");
    } else {
        printf("\nRegistration failed! Please check your input.\n");
        return 1;
    }

    display_users(DB_FILE);

    return 0;
}