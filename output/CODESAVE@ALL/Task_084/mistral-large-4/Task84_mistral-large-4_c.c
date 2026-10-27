#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

#define SESSION_ID_BYTES 16
#define SESSION_ID_LENGTH (SESSION_ID_BYTES * 2)

int generate_session_id(char *buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size < SESSION_ID_LENGTH + 1) {
        return -1;
    }

    unsigned char bytes[SESSION_ID_BYTES];
    int have_random = 0;

    FILE *fp = fopen("/dev/urandom", "rb");
    if (fp != NULL) {
        if (fread(bytes, 1, SESSION_ID_BYTES, fp) == SESSION_ID_BYTES) {
            have_random = 1;
        }
        fclose(fp);
    }

    if (!have_random) {
        unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)clock() ^ (unsigned int)getpid();
        srand(seed);
        for (int i = 0; i < SESSION_ID_BYTES; i++) {
            bytes[i] = (unsigned char)(rand() & 0xFF);
        }
    }

    const char *hex = "0123456789abcdef";
    for (int i = 0; i < SESSION_ID_BYTES; i++) {
        buffer[i * 2] = hex[(bytes[i] >> 4) & 0x0F];
        buffer[i * 2 + 1] = hex[bytes[i] & 0x0F];
    }
    buffer[SESSION_ID_LENGTH] = '\0';

    return 0;
}

int main(void) {
    char session_id[SESSION_ID_LENGTH + 1];

    if (generate_session_id(session_id, sizeof(session_id)) != 0) {
        fprintf(stderr, "Error: Failed to generate session ID\n");
        return 1;
    }

    printf("%s\n", session_id);
    return 0;
}