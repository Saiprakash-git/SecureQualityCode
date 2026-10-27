```c
/*
 * sensitive_store.c
 * Stores personal and payment data with input validation, masked display,
 * and encrypted-at-rest file storage.
 *
 * Build: cc -std=c99 -Wall -Wextra -o sensitive_store sensitive_store.c
 * Run:   ./sensitive_store <passphrase> [data_file]
 *
 * NOTE: The at-rest cipher below is a simple XOR keystream derived from a
 * passphrase. It illustrates the structure of encrypted storage but is not a
 * vetted cryptographic construction. For production use, replace xor_cipher()
 * with an authenticated cipher such as AES-GCM or XChaCha20-Poly1305 from a
 * reviewed library (for example libsodium or OpenSSL).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_RECORDS   100
#define NAME_LEN      64
#define EMAIL_LEN     64
#define CARD_LEN      20
#define SSN_LEN       12
#define MAGIC         0x53534431u  /* "SSD1" */

typedef struct {
    char name[NAME_LEN];
    char email[EMAIL_LEN];
    char card[CARD_LEN];
    char ssn[SSN_LEN];
    unsigned char used;
} Record;

typedef struct {
    Record items[MAX_RECORDS];
    size_t count;
} Store;

/* ---------- Validation ---------- */

static int luhn_valid(const char *num)
{
    int sum = 0, alt = 0;
    size_t len = strlen(num);
    if (len < 13 || len > 19) return 0;
    for (size_t i = len; i-- > 0;) {
        if (!isdigit((unsigned char)num[i])) return 0;
        int d = num[i] - '0';
        if (alt) {
            d *= 2;
            if (d > 9) d -= 9;
        }
        sum += d;
        alt = !alt;
    }
    return sum % 10 == 0;
}

static int validate_ssn(const char *ssn)
{
    /* Accepts format NNN-NN-NNNN */
    if (strlen(ssn) != 11) return 0;
    for (int i = 0; i < 11; i++) {
        if (i == 3 || i == 6) {
            if (ssn[i] != '-') return 0;
        } else if (!isdigit((unsigned char)ssn[i])) {
            return 0;
        }
    }
    return 1;
}

static int validate_email(const char *email)
{
    const char *at = strchr(email, '@');
    if (!at || at == email) return 0;
    if (!strchr(at, '.')) return 0;
    return strlen(email) < EMAIL_LEN;
}

static void strip_newline(char *s)
{
    size_t n = strcspn(s, "\r\n");
    s[n] = '\0';
}

/* ---------- Masking for display ---------- */

static void mask_card(const char *card, char *out, size_t outsz)
{
    size_t len = strlen(card);
    size_t i;
    if (outsz == 0) return;
    if (len <= 4) {
        snprintf(out, outsz, "****");
        return;
    }
    for (i = 0; i < len - 4 && i + 1 < outsz; i++) out[i] = '*';
    snprintf(out + i, outsz - i, "%s", card + len - 4);
}

static void mask_ssn(const char *ssn, char *out, size_t outsz)
{
    snprintf(out, outsz, "***-**-%s", ssn + 7);
}

/* ---------- Store operations ---------- */

static int store_add(Store *s, const char *name, const char *email,
                     const char *card, const char *ssn)
{
    if (!s || s->count >= MAX_RECORDS) return -1;
    if (!name || !*name || strlen(name) >= NAME_LEN) return -1;
    if (!validate_email(email)) return -1;
    if (!luhn_valid(card)) return -1;
    if (!validate_ssn(ssn)) return -1;

    Record *r = &s->items[s->count];
    memset(r, 0, sizeof(*r));
    strcpy(r->name, name);
    strcpy(r->email, email);
    strcpy(r->card, card);
    strcpy(r->ssn, ssn);
    r->used = 1;
    s->count++;
    return (int)(s->count - 1);
}

static int store_delete(Store *s, size_t index)
{
    if (!s || index >= s->count) return -1;
    for (size_t i = index; i + 1 < s->count; i++) {
        s->items[i] = s->items[i + 1];
    }
    s->count--;
    memset(&s->items[s->count], 0, sizeof(Record));
    return 0;
}

static void store_list(const Store *s, char *out, size_t outsz)
{
    size_t used = 0;
    char mcard[CARD_LEN], mssn[SSN_LEN + 8];
    out[0] = '\0';
    for (size_t i = 0; i < s->count; i++) {
        mask_card(s->items[i].card, mcard, sizeof(mcard));
        mask_ssn(s->items[i].ssn, mssn, sizeof(mssn));
        int n = snprintf(out + used, outsz - used,
                         "[%zu] %s <%s> card:%s ssn:%s\n",
                         i, s->items[i].name, s->items[i].email,
                         mcard, mssn);
        if (n < 0 || (size_t)n >= outsz - used) break;
        used += (size_t)n;
    }
}

/* ---------- At-rest protection ---------- */

static void xor_cipher(unsigned char *buf, size_t len,
                       const char *key, unsigned long long nonce)
{
    size_t klen = strlen(key);
    unsigned long long state = nonce ^ 0x9E3779B97F4A7C15ULL;
    if (klen == 0) return;
    for (size_t i = 0; i < len; i++) {
        state = state * 6364136223846793005ULL + 1442695040888963407ULL;
        buf[i] ^= (unsigned char)key[i % klen] ^ (unsigned char)(state >> 56);
    }
}

static int store_save(const Store *s, const char *path, const char *key)
{
    FILE *f;
    unsigned int magic = MAGIC;
    unsigned long long nonce;
    size_t total = sizeof(Record) * s->count;
    unsigned char *buf;

    if (total == 0) total = 1;
    buf = malloc(total);
    if (!buf) return -1;
    if (s->count) memcpy(buf, s->items, sizeof(Record) * s->count);

    nonce = (unsigned long long)time(NULL) ^ ((unsigned long long)rand() << 16);
    xor_cipher(buf, sizeof(Record) * s->count, key, nonce);

    f = fopen(path, "wb");
    if (!f) { free(buf); return -1; }
    fwrite(&magic, sizeof(magic), 1, f);
    fwrite(&nonce, sizeof(nonce), 1, f);
    fwrite(&s->count, sizeof(s->count), 1, f);
    fwrite(buf, 1, sizeof(Record) * s->count, f);
    fclose(f);

    memset(buf, 0, total);
    free(buf);
    return 0;
}

static int store_load(Store *s, const char *path, const char *key)
{
    FILE *f;
    unsigned int magic;
    unsigned long long nonce;
    size_t count;

    f = fopen(path, "rb");
    if (!f) return -1;
    if (fread(&magic, sizeof(magic), 1, f) != 1 || magic != MAGIC ||
        fread(&nonce, sizeof(nonce), 1, f) != 1 ||
        fread(&count, sizeof(count), 1, f) != 1 ||
        count > MAX_RECORDS) {
        fclose(f);
        return -1;
    }
    if (fread(s->items, sizeof(Record), count, f) != count) {
        fclose(f);
        return -1;
    }
    fclose(f);

    xor_cipher((unsigned char *)s->items, sizeof(Record) * count, key, nonce);
    s->count = count;
    return 0;
}

/* ---------- Interactive front end ---------- */

static void prompt(const char *label, char *buf, size_t sz)
{
    printf("%s", label);
    fflush(stdout);
    if (!fgets(buf, (int)sz, stdin)) {
        buf[0] = '\0';
        return;
    }
    strip_newline(buf);
}

int main(int argc, char *argv[])
{
    static Store store;
    const char *path = "sensitive_data.bin";
    char choice[8], name[NAME_LEN], email[EMAIL_LEN];
    char card[CARD_LEN], ssn[SSN_LEN], listing[8192], idx[16];

    if (argc < 2) {
        fprintf(stderr, "usage: %s <passphrase> [data_file]\n", argv[0]);
        return 1;
    }
    if (argc >= 3) path = argv[2];
    srand((unsigned)time(NULL));

    for (;;) {
        printf("\n1) Add  2) List  3) Delete  4) Save  5) Load  6) Quit\n");
        prompt("> ", choice, sizeof(choice));

        if (choice[0] == '1') {
            prompt("Name: ", name, sizeof(name));
            prompt("Email: ", email, sizeof(email));
            prompt("Card number (digits): ", card, sizeof(card));
            prompt("SSN (NNN-NN-NNNN): ", ssn, size