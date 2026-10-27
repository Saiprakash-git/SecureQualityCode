#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#define AUTH_USERNAME_MAX 64
#define AUTH_PASSWORD_MAX 1024
#define AUTH_SALT_SIZE 16
#define AUTH_HASH_SIZE 32
#define AUTH_PBKDF2_ITERATIONS 600000

typedef struct {
    char username[AUTH_USERNAME_MAX + 1];
    unsigned char salt[AUTH_SALT_SIZE];
    unsigned char password_hash[AUTH_HASH_SIZE];
} AuthUser;

typedef enum {
    AUTH_REGISTERED = 0,
    AUTH_USER_EXISTS,
    AUTH_STORE_FULL,
    AUTH_INVALID_ARGUMENT,
    AUTH_CRYPTO_ERROR
} AuthRegisterResult;

typedef enum {
    AUTH_LOGIN_OK = 0,
    AUTH_LOGIN_INVALID_CREDENTIALS,
    AUTH_LOGIN_CRYPTO_ERROR
} AuthLoginResult;

static int auth_username_length(const char *username, size_t *length)
{
    size_t i;

    if (username == NULL || length == NULL)
        return 0;

    for (i = 0; i <= AUTH_USERNAME_MAX; ++i) {
        unsigned char c = (unsigned char)username[i];

        if (c == '\0') {
            if (i == 0)
                return 0;
            *length = i;
            return 1;
        }

        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '_' || c == '-' || c == '.'))
            return 0;
    }

    return 0;
}

static int auth_password_hash(const unsigned char *password,
                              size_t password_length,
                              const unsigned char salt[AUTH_SALT_SIZE],
                              unsigned char output[AUTH_HASH_SIZE])
{
    if (password == NULL || salt == NULL || output == NULL ||
        password_length > AUTH_PASSWORD_MAX)
        return 0;

    return PKCS5_PBKDF2_HMAC((const char *)password,
                             (int)password_length,
                             salt,
                             AUTH_SALT_SIZE,
                             AUTH_PBKDF2_ITERATIONS,
                             EVP_sha256(),
                             AUTH_HASH_SIZE,
                             output) == 1;
}

static int auth_stored_username_matches(const AuthUser *user,
                                        const char *username)
{
    if (user == NULL || username == NULL ||
        memchr(user->username, '\0', sizeof(user->username)) == NULL)
        return 0;

    return strcmp(user->username, username) == 0;
}

AuthRegisterResult auth_register(AuthUser *users,
                                 size_t *user_count,
                                 size_t capacity,
                                 const char *username,
                                 const unsigned char *password,
                                 size_t password_length)
{
    size_t username_length;
    size_t i;
    AuthUser pending;

    if (users == NULL || user_count == NULL ||
        *user_count > capacity ||
        !auth_username_length(username, &username_length) ||
        password == NULL ||
        password_length < 12 ||
        password_length > AUTH_PASSWORD_MAX)
        return AUTH_INVALID_ARGUMENT;

    for (i = 0; i < *user_count; ++i) {
        if (auth_stored_username_matches(&users[i], username))
            return AUTH_USER_EXISTS;
    }

    if (*user_count == capacity)
        return AUTH_STORE_FULL;

    memset(&pending, 0, sizeof(pending));
    memcpy(pending.username, username, username_length);

    if (RAND_bytes(pending.salt, AUTH_SALT_SIZE) != 1 ||
        !auth_password_hash(password, password_length,
                            pending.salt, pending.password_hash)) {
        OPENSSL_cleanse(&pending, sizeof(pending));
        return AUTH_CRYPTO_ERROR;
    }

    users[*user_count] = pending;
    ++(*user_count);
    OPENSSL_cleanse(&pending, sizeof(pending));
    return AUTH_REGISTERED;
}

AuthLoginResult auth_login(const AuthUser *users,
                           size_t user_count,
                           const char *username,
                           const unsigned char *password,
                           size_t password_length,
                           size_t *authenticated_index)
{
    static const unsigned char dummy_salt[AUTH_SALT_SIZE] = {
        0x7a, 0x31, 0x9d, 0x04, 0x56, 0xc2, 0x88, 0xe1,
        0x13, 0xb7, 0x4f, 0xa9, 0x60, 0x2d, 0x83, 0x5c
    };
    static const unsigned char dummy_hash[AUTH_HASH_SIZE] = { 0 };
    unsigned char calculated_hash[AUTH_HASH_SIZE];
    size_t i;
    size_t found_index = 0;
    int username_valid;
    int found = 0;
    int hash_ok;

    if (authenticated_index != NULL)
        *authenticated_index = 0;

    if (password == NULL || password_length > AUTH_PASSWORD_MAX)
        return AUTH_LOGIN_INVALID_CREDENTIALS;

    username_valid = auth_username_length(username, &i);

    if (username_valid && users != NULL) {
        for (i = 0; i < user_count; ++i) {
            if (auth_stored_username_matches(&users[i], username)) {
                found = 1;
                found_index = i;
                break;
            }
        }
    }

    if (found) {
        hash_ok = auth_password_hash(password, password_length,
                                     users[found_index].salt,
                                     calculated_hash);
    } else {
        hash_ok = auth_password_hash(password, password_length,
                                     dummy_salt, calculated_hash);
    }

    if (!hash_ok) {
        OPENSSL_cleanse(calculated_hash, sizeof(calculated_hash));
        return AUTH_LOGIN_CRYPTO_ERROR;
    }

    if (found &&
        CRYPTO_memcmp(calculated_hash,
                      users[found_index].password_hash,
                      AUTH_HASH_SIZE) == 0) {
        if (authenticated_index != NULL)
            *authenticated_index = found_index;
        OPENSSL_cleanse(calculated_hash, sizeof(calculated_hash));
        return AUTH_LOGIN_OK;
    }

    if (!found)
        (void)CRYPTO_memcmp(calculated_hash, dummy_hash, AUTH_HASH_SIZE);

    OPENSSL_cleanse(calculated_hash, sizeof(calculated_hash));
    return AUTH_LOGIN_INVALID_CREDENTIALS;
}

void auth_user_clear(AuthUser *user)
{
    if (user != NULL)
        OPENSSL_cleanse(user, sizeof(*user));
}