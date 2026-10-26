#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

#define MAX_PATH_LEN 1024
#define BUFFER_SIZE 8192
#define NUM_WORKERS 4

typedef struct QueueNode {
    char filepath[MAX_PATH_LEN];
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *head;
    QueueNode *tail;
    int shutdown;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} WorkQueue;

typedef struct {
    long total_bytes;
    long line_count;
    unsigned int checksum;
    int success;
} ProcessResult;

typedef struct {
    int thread_id;
    WorkQueue *queue;
    pthread_mutex_t *log_mutex;
} WorkerArgs;

int queue_init(WorkQueue *q) {
    if (!q) return -1;
    q->head = NULL;
    q->tail = NULL;
    q->shutdown = 0;
    if (pthread_mutex_init(&q->mutex, NULL) != 0) return -1;
    if (pthread_cond_init(&q->cond, NULL) != 0) {
        pthread_mutex_destroy(&q->mutex);
        return -1;
    }
    return 0;
}

int queue_push(WorkQueue *q, const char *filepath) {
    if (!q || !filepath) return -1;

    QueueNode *new_node = (QueueNode *)malloc(sizeof(QueueNode));
    if (!new_node) return -1;

    strncpy(new_node->filepath, filepath, MAX_PATH_LEN - 1);
    new_node->filepath[MAX_PATH_LEN - 1] = '\0';
    new_node->next = NULL;

    pthread_mutex_lock(&q->mutex);
    if (q->shutdown) {
        pthread_mutex_unlock(&q->mutex);
        free(new_node);
        return -1;
    }

    if (q->tail == NULL) {
        q->head = new_node;
        q->tail = new_node;
    } else {
        q->tail->next = new_node;
        q->tail = new_node;
    }

    pthread_cond_signal(&q->cond);
    pthread_mutex_unlock(&q->mutex);
    return 0;
}

int queue_pop(WorkQueue *q, char *buffer, size_t buf_len) {
    if (!q || !buffer || buf_len == 0) return -1;

    pthread_mutex_lock(&q->mutex);
    while (q->head == NULL && !q->shutdown) {
        pthread_cond_wait(&q->cond, &q->mutex);
    }

    if (q->head == NULL && q->shutdown) {
        pthread_mutex_unlock(&q->mutex);
        return 0;
    }

    QueueNode *node = q->head;
    q->head = node->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }

    strncpy(buffer, node->filepath, buf_len - 1);
    buffer[buf_len - 1] = '\0';

    free(node);
    pthread_mutex_unlock(&q->mutex);
    return 1;
}

void queue_shutdown(WorkQueue *q) {
    if (!q) return;
    pthread_mutex_lock(&q->mutex);
    q->shutdown = 1;
    pthread_cond_broadcast(&q->cond);
    pthread_mutex_unlock(&q->mutex);
}

void queue_destroy(WorkQueue *q) {
    if (!q) return;
    pthread_mutex_lock(&q->mutex);
    QueueNode *current = q->head;
    while (current != NULL) {
        QueueNode *next = current->next;
        free(current);
        current = next;
    }
    q->head = NULL;
    q->tail = NULL;
    pthread_mutex_unlock(&q->mutex);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->cond);
}

ProcessResult process_file(const char *filepath) {
    ProcessResult result = {0, 0, 0, 0};
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        return result;
    }

    unsigned char buffer[BUFFER_SIZE];
    size_t bytes_read;

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        result.total_bytes += (long)bytes_read;
        for (size_t i = 0; i < bytes_read; ++i) {
            if (buffer[i] == '\n') {
                result.line_count++;
            }
            result.checksum = (result.checksum * 31) + buffer[i];
        }
    }

    if (ferror(fp)) {
        fclose(fp);
        result.success = 0;
        return result;
    }

    fclose(fp);
    result.success = 1;
    return result;
}

void *worker_routine(void *arg) {
    WorkerArgs *wargs = (WorkerArgs *)arg;
    char filepath[MAX_PATH_LEN];

    while (1) {
        int ret = queue_pop(wargs->queue, filepath, sizeof(filepath));
        if (ret <= 0) {
            break;
        }

        ProcessResult res = process_file(filepath);

        pthread_mutex_lock(wargs->log_mutex);
        if (res.success) {
            printf("[Thread %d] Processed: %s | Bytes: %ld | Lines: %ld | Checksum: 0x%08X\n",
                   wargs->thread_id, filepath, res.total_bytes, res.line_count, res.checksum);
        } else {
            fprintf(stderr, "[Thread %d] Error processing: %s\n", wargs->thread_id, filepath);
        }
        pthread_mutex_unlock(wargs->log_mutex);
    }

    return NULL;
}

int scan_directory_and_enqueue(const char *dir_path, WorkQueue *queue) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror("Failed to open directory");
        return -1;
    }

    struct dirent *entry;
    char fullpath[MAX_PATH_LEN];
    struct stat statbuf;
    int enqueued_count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir_path, entry->d_name);

        if (stat(fullpath, &statbuf) == 0) {
            if (S_ISREG(statbuf.st_mode)) {
                if (queue_push(queue, fullpath) == 0) {
                    enqueued_count++;
                }
            }
        }
    }

    closedir(dir);
    return enqueued_count;
}

int create_sample_files(const char *dir_path, int count) {
    struct stat st;
    if (stat(dir_path, &st) == -1) {
        if (mkdir(dir_path, 0755) != 0) {
            return -1;
        }
    }

    for (int i = 0; i < count; ++i) {
        char filename[MAX_PATH_LEN];
        snprintf(filename, sizeof(filename), "%s/sample_%d.txt", dir_path, i + 1);
        FILE *f = fopen(filename, "w");
        if (!f) continue;

        int lines = (i + 1) * 20;
        for (int j = 1; j <= lines; ++j) {
            fprintf(f, "Sample file #%d, payload line number %d.\n", i + 1, j);
        }
        fclose(f);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    const char *target_directory = "demo_shared_dir";
    int created_demo_data = 0;

    if (argc >= 2) {
        target_directory = argv[1];
    } else {
        if (create_sample_files(target_directory, 8) == 0) {
            created_demo_data = 1;
        }
    }

    WorkQueue queue;
    if (queue_init(&queue) != 0) {
        fprintf(stderr, "Failed to initialize work queue.\n");
        return EXIT_FAILURE;
    }

    pthread_mutex_t log_mutex;
    if (pthread_mutex_init(&log_mutex, NULL) != 0) {
        queue_destroy(&queue);
        return EXIT_FAILURE;
    }

    pthread_t threads[NUM_WORKERS];
    WorkerArgs worker_args[NUM_WORKERS];

    for (int i = 0; i < NUM_WORKERS; ++i) {
        worker_args[i].thread_id = i + 1;
        worker_args[i].queue = &queue;
        worker_args[i].log_mutex = &log_mutex;

        if (pthread_create(&threads[i], NULL, worker_routine, &worker_args[i]) != 0) {
            fprintf(stderr, "Failed to create thread %d.\n", i);
            queue_