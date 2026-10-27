#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

int main(void)
{
    int fd;
    const char *content = "#!/bin/sh\necho Hello from script.sh\n";

    fd = open("script.sh", O_CREAT | O_WRONLY | O_TRUNC, 0755);
    if (fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }

    if (write(fd, content, sizeof(content) - 1) < 0) {
        perror("write");
        close(fd);
        return EXIT_FAILURE;
    }

    close(fd);

    if (chmod("script.sh", 0755) != 0) {
        perror("chmod");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}