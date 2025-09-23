#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "lib-misc.h"

int main() {
    int fd = open("./file-da-aprire.txt", O_CREAT|O_RDWR, S_IWUSR|S_IRUSR|S_IXUSR);

    if ((write(fd, (void *)"aaaaaaaaaaaaaaaaaaaa\n", 30)) == -1) {
        exit_with_err_msg("write");
    }

    int fd2;

    dup2(1, fd);
    if ((write(fd, (void *)"bbbbbbbbbbbbbbbbbbbb\n", 30)) == -1) {
        exit_with_err_msg("write");
    }

    close(fd);
}