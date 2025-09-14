#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("wrong usage. correct syntax: %s <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    struct stat file_stats;
    if (stat(argv[1], &file_stats) == -1) {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    printf("name\t\t\t: %s\n", argv[1]); // file name
    printf("i-node number\t\t: %ld\n", file_stats.st_ino); // i-node number
    printf("date of creation\t: %s", ctime(&file_stats.st_ctime)); // date of the last modify to the i-node. is a good approx of the file creation.
    printf("size\t\t\t: %jd bytes\n", file_stats.st_size); // size in bytes
    printf(" |\t\t\t: %jd kb\n",file_stats.st_size/1000); // kilobytes
    printf(" |\t\t\t: %jd mb\n",file_stats.st_size/1000000); // megabytes
}