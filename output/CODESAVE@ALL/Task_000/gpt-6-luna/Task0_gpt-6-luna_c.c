#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    char *path;
    dev_t device;
    ino_t inode;
    pthread_mutex_t mutex;
    uint64_t bytes;
    uint64_t checksum;
} Task;

typedef struct {
    Task **tasks;
    size_t count;
    size_t next;
    pthread_mutex_t queue_mutex;
    pthread_mutex_t output_mutex;
    int failed;
} WorkQueue;

static int add_task(Task ***tasks, size_t *count, const char *directory,
                    const char *name, const struct stat *st)
{
    size_t directory_length = strlen(directory);
    size_t name_length = strlen(name);
    int needs_slash = directory_length > 0 &&
                      directory[directory_length - 1] != '/';

    if (directory_length > SIZE_MAX - name_length - (size_t)needs_slash - 1)
        return ENAMETOOLONG;

    size_t path_length = directory_length + (size_t)needs_slash + name_length;
    char *path = malloc(path_length + 1);
    if (path == NULL)
        return ENOMEM;

    memcpy(path, directory, directory_length);
    size_t offset = directory_length;
    if (needs_slash)
        path[offset++] = '/';
    memcpy(path + offset, name, name_length);
    path[path_length] = '\0';

    Task *task = calloc(1, sizeof(*task));
    if (task == NULL) {
        free(path);
        return ENOMEM;
    }

    int rc = pthread_mutex_init(&task->mutex, NULL);
    if (rc != 0) {
        free(path);
        free(task);
        return rc;
    }

    task->path = path;
    task->device = st->st_dev;
    task->inode = st->st_ino;

    if (*count == SIZE_MAX / sizeof(**tasks)) {
        pthread_mutex_destroy(&task->mutex);
        free(path);
        free(task);
        return ENOMEM;
    }

    Task **grown = realloc(*tasks, (*count + 1) * sizeof(**tasks));
    if (grown == NULL) {
        pthread_mutex_destroy(&task->mutex);
        free(path);
        free(task);
        return ENOMEM;
    }

    *tasks = grown;
    (*tasks)[(*count)++] = task;
    return 0;
}

static int scan_directory(const char *directory, Task ***tasks, size_t *count,
                          int *scan_failed)
{
    DIR *dir = opendir(directory);
    if (dir == NULL)
        return errno;

    int fatal_error = 0;
    int directory_fd = dirfd(dir);

    for (;;) {
        errno = 0;
        struct dirent *entry = readdir(dir);
        if (entry == NULL) {
            if (errno != 0)
                fatal_error = errno;
            break;
        }

        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        struct stat st;
        if (fstatat(directory_fd, entry->d_name, &st,
                    AT_SYMLINK_NOFOLLOW) != 0) {
            *scan_failed = 1;
            continue;
        }

        if (!S_ISREG(st.st_mode))
            continue;

        int rc = add_task(tasks, count, directory, entry->d_name, &st);
        if (rc != 0) {
            fatal_error = rc;
            break;
        }
    }

    if (closedir(dir) != 0 && fatal_error == 0)
        fatal_error = errno;

    return fatal_error;
}

static int process_file(Task *task)
{
    int fd = open(task->path, O_RDONLY | O_CLOEXEC);
    if (fd < 0)
        return errno;

    struct stat st;
    if (fstat(fd, &st) != 0) {
        int error = errno;
        close(fd);
        return error;
    }

    if (!S_ISREG(st.st_mode) || st.st_dev != task->device ||
        st.st_ino != task->inode) {
        close(fd);
        return EIO;
    }

    unsigned char buffer[1024 * 1024];
    uint64_t bytes = 0;
    uint64_t checksum = UINT64_C(14695981039346656037);

    for (;;) {
        ssize_t amount = read(fd, buffer, sizeof(buffer));
        if (amount < 0) {
            if (errno == EINTR)
                continue;
            int error = errno;
            close(fd);
            return error;
        }
        if (amount == 0)
            break;

        bytes += (uint64_t)amount;
        for (ssize_t i = 0; i < amount; ++i) {
            checksum ^= buffer[i];
            checksum *= UINT64_C(1099511628211);
        }
    }

    if (close(fd) != 0)
        return errno;

    task->bytes = bytes;
    task->checksum = checksum;
    return 0;
}

static void *worker(void *argument)
{
    WorkQueue *queue = argument;

    for (;;) {
        pthread_mutex_lock(&queue->queue_mutex);
        if (queue->next >= queue->count) {
            pthread_mutex_unlock(&queue->queue_mutex);
            break;
        }

        Task *task = queue->tasks[queue->next++];
        pthread_mutex_unlock(&queue->queue_mutex);

        int rc = pthread_mutex_lock(&task->mutex);
        int error = rc == 0 ? process_file(task) : rc;
        if (rc == 0)
            pthread_mutex_unlock(&task->mutex);

        pthread_mutex_lock(&queue->output_mutex);
        if (error != 0) {
            fprintf(stderr, "Error processing %s: %s\n",
                    task->path, strerror(error));
            queue->failed = 1;
        } else {
            printf("%s bytes=%llu checksum=%016llx\n",
                   task->path,
                   (unsigned long long)task->bytes,
                   (unsigned long long)task->checksum);
        }
        pthread_mutex_unlock(&queue->output_mutex);
    }

    return NULL;
}

static void free_tasks(Task **tasks, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        pthread_mutex_destroy(&tasks[i]->mutex);
        free(tasks[i]->path);
        free(tasks[i]);
    }
    free(tasks);
}

static int parse_thread_count(const char *text, size_t *result)
{
    if (text == NULL || *text == '\0')
        return EINVAL;

    errno = 0;
    char *end = NULL;
    unsigned long long value = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value == 0 ||
        value > 256)
        return EINVAL;

    *result = (size_t)value;
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "Usage: %s DIRECTORY [THREADS]\n", argv[0]);
        return 2;
    }

    size_t thread_count;
    if (argc == 3) {
        if (parse_thread_count(argv[2], &thread_count) != 0) {
            fprintf(stderr, "THREADS must be an integer from 1 to 256\n");
            return 2;
        }
    } else {
        long online = sysconf(_SC_NPROCESSORS_ONLN);
        thread_count = online > 0 ? (size_t)online : 1;
        if (thread_count > 256)
            thread_count = 256;
    }

    Task **tasks = NULL;
    size_t task_count = 0;
    int scan_failed = 0;
    int scan_error = scan_directory(argv[1], &tasks, &task_count,
                                    &scan_failed);
    if (scan_error != 0) {
        fprintf(stderr, "Error scanning directory %s: %s\n",
                argv[1], strerror(scan_error));
        free_tasks(tasks, task_count);
        return 1;
    }

    if (task_count == 0) {
        if (scan_failed)
            fprintf(stderr, "Some directory entries could not be inspected\n");
        free_tasks(tasks, task_count);
        return scan_failed ? 1 : 0;
    }

    if (thread_count > task_count)
        thread_count = task_count;

    WorkQueue queue = {
        .tasks = tasks,
        .count = task_count,
        .next = 0,
        .failed = scan_failed
    };

    int rc = pthread_mutex_init(&queue.queue_mutex, NULL);
    if (rc != 0) {
        fprintf(stderr, "Could not initialize queue mutex: %s\n", strerror(rc));
        free_tasks(tasks, task_count);
        return 1;
    }

    rc = pthread_mutex_init(&queue.output_mutex, NULL);
    if (rc != 0) {
        fprintf(stderr, "Could not initialize output mutex: %s\n", strerror(rc));
        pthread_mutex_destroy(&queue.queue_mutex);
        free_tasks(tasks, task_count);
        return 1;
    }

    pthread_t *threads = NULL;
    size_t created = 0;
    if (thread_count > 1) {
        threads = calloc(thread_count - 1, sizeof(*threads));
        if (threads == NULL) {
            fprintf(stderr, "Could not allocate thread handles\n");