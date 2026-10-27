#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void)
{
    static const char contents[] = "#!/bin/sh\n";
    int fd = open("script.sh", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1)
        return 1;

    size_t written = 0;
    while (written < sizeof(contents) - 1) {
        ssize_t n = write(fd, contents + written, sizeof(contents) - 1 - written);
        if (n == -1) {
            if (errno == EINTR)
                continue;
            close(fd);
            return 1;
        }
        written += (size_t)n;
    }

    if (fchmod(fd, 0755) == -1) {
        close(fd);
        return 1;
    }

    return close(fd) == -1 ? 1 : 0;
}