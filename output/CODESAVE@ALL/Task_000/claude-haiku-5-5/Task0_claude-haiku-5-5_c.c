#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_THREADS 64
#define DEFAULT_THREADS 4
#define DEFAULT_PASSES 1
#define PATH_BUF 4096
#define READ_CHUNK 65536

typedef struct {
    unsigned long long bytes;
    unsigned long long lines;
    unsigned long long words;
} counts_t;

typedef struct {
    char *path;
    pthread_mutex_t lock;   /* held only while this file is being processed */
    counts_t totals;        /* accumulated across passes, protected by lock */
    unsigned long passes_done;
} file_entry_t;

typedef struct {
    file_entry_t *files;
    size_t nfiles;
    size_t total_jobs;      /* nfiles * passes */
    size_t next_job;        /* protected by queue_lock */
    pthread_mutex_t queue_lock;
    pthread_mutex_t out_lock; /* protects console output and global totals */
    counts_t global;
    unsigned long long errors;
} shared_ctx_t;

typedef struct {
    shared_ctx_t *ctx;
    int thread_id;
    unsigned long jobs_done;
} worker_arg_t;

/* Pure counting routine: reads a file and fills counts. Returns 0 on success, -1 on error. */
static int count_file(const char *path, counts_t *out)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
        return -1;

    counts_t c = {0, 0, 0};
    unsigned char *buf = malloc(READ_CHUNK);
    if (!buf) {
        fclose(fp);
        return -1;
    }

    int in_word = 0;
    size_t n;
    while ((n = fread(buf, 1, READ_CHUNK, fp)) > 0) {
        for (size_t i = 0; i < n; i++) {
            unsigned char ch = buf[i];
            c.bytes++;
            if (ch == '\n')
                c.lines++;
            if (isspace(ch)) {
                in_word = 0;
            } else if (!in_word) {
                in_word = 1;
                c.words++;
            }
        }
    }

    int err = ferror(fp);
    free(buf);
    fclose(fp);
    if (err)
        return -1;

    *out = c;
    return 0;
}

/* Processes one file. Locks only this file's mutex, never another lock while held. */
static int process_file(shared_ctx_t *ctx, file_entry_t *f, int thread_id)
{
    counts_t c;
    int rc;

    pthread_mutex_lock(&f->lock);
    rc = count_file(f->path, &c);
    if (rc == 0) {
        f->totals.bytes += c.bytes;
        f->totals.lines += c.lines;
        f->totals.words += c.words;
        f->passes_done++;
    }
    pthread_mutex_unlock(&f->lock);

    /* Output lock taken after file lock is released: no nested locking. */
    pthread_mutex_lock(&ctx->out_lock);
    if (rc == 0) {
        ctx->global.bytes += c.bytes;
        ctx->global.lines += c.lines;
        ctx->global.words += c.words;
        printf("[thread %2d] %-60s bytes=%llu lines=%llu words=%llu\n",
               thread_id, f->path, c.bytes, c.lines, c.words);
    } else {
        ctx->errors++;
        printf("[thread %2d] %-60s ERROR: %s\n",
               thread_id, f->path, strerror(errno));
    }
    pthread_mutex_unlock(&ctx->out_lock);

    return rc;
}

static void *worker_main(void *arg)
{
    worker_arg_t *w = (worker_arg_t *)arg;
    shared_ctx_t *ctx = w->ctx;

    for (;;) {
        size_t job;

        pthread_mutex_lock(&ctx->queue_lock);
        if (ctx->next_job >= ctx->total_jobs) {
            pthread_mutex_unlock(&ctx->queue_lock);
            break;
        }
        job = ctx->next_job++;
        pthread_mutex_unlock(&ctx->queue_lock);

        size_t idx = job % ctx->nfiles;
        process_file(ctx, &ctx->files[idx], w->thread_id);
        w->jobs_done++;
    }
    return NULL;
}

/* Scans a directory for regular files. Returns number of files found, or -1 on error. */
static long collect_files(const char *dir, file_entry_t **out_files)
{
    DIR *d = opendir(dir);
    if (!d)
        return -1;

    size_t cap = 64, n = 0;
    file_entry_t *files = malloc(cap * sizeof(*files));
    if (!files) {
        closedir(d);
        return -1;
    }

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
            continue;

        char full[PATH_BUF];
        int len = snprintf(full, sizeof(full), "%s/%s", dir, ent->d_name);
        if (len < 0 || (size_t)len >= sizeof(full))
            continue;

        struct stat st;
        if (stat(full, &st) != 0 || !S_ISREG(st.st_mode))
            continue;

        if (n == cap) {
            cap *= 2;
            file_entry_t *tmp = realloc(files, cap * sizeof(*files));
            if (!tmp) {
                for (size_t i = 0; i < n; i++) {
                    free(files[i].path);
                    pthread_mutex_destroy(&files[i].lock);
                }
                free(files);
                closedir(d);
                return -1;
            }
            files = tmp;
        }

        files[n].path = strdup(full);
        if (!files[n].path)
            continue;
        memset(&files[n].totals, 0, sizeof(files[n].totals));
        files[n].passes_done = 0;
        pthread_mutex_init(&files[n].lock, NULL);
        n++;
    }
    closedir(d);

    *out_files = files;
    return (long)n;
}

static void free_files(file_entry_t *files, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        free(files[i].path);
        pthread_mutex_destroy(&files[i].lock);
    }
    free(files);
}

static int parse_long(const char *s, long min, long max, long *out)
{
    char *end;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno != 0 || *end != '\0' || v < min || v > max)
        return -1;
    *out = v;
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <directory> [threads=%d] [passes=%d]\n",
                argv[0], DEFAULT_THREADS, DEFAULT_PASSES);
        return EXIT_FAILURE;
    }

    long threads = DEFAULT_THREADS, passes = DEFAULT_PASSES;
    if (argc >= 3 && parse_long(argv[2], 1, MAX_THREADS, &threads) != 0) {
        fprintf(stderr, "