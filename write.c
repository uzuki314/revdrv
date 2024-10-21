#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_usage()
{
    printf(
        "Usage: write STRING [OPTION]... FILE\n"
        "Examples:\n"
        "  write \"test\" -O TRUNC ./testfile\n"
        "  write \"hello world\" -O APPEND ./testfile\n"
        "  write \"$$$\" -s -2 -S END ./testfile\n"
        "  -O, --open-flags  flags    openflags=O_RDWR|O_CREAT|flags\n"
        "  -s, --lseek-offset num      set lseek offset to num\n"
        "  -S, --lseek-whence whence   set lseek whence to SEEK_[whence], "
        "default SEEK_SET\n");
}

int main(int argc, char *argv[])
{
    int opt;
    int open_flags = O_RDWR | O_CREAT;
    off_t lseek_offset = 0;
    int lseek_whence = SEEK_SET;

    if (argc < 3) {
        fprintf(stderr, "Not enough arguments\n");
        print_usage();
        return EXIT_FAILURE;
    }

    while ((opt = getopt(argc, argv, "O:s:S:")) != -1) {
        switch (opt) {
        case 'O':
            if (strcmp(optarg, "TRUNC") == 0) {
                open_flags |= O_TRUNC;
            } else if (strcmp(optarg, "APPEND") == 0) {
                open_flags |= O_APPEND;
            } else {
                fprintf(stderr, "Invalid open flag: %s\n", optarg);
                print_usage();
                return EXIT_FAILURE;
            }
            break;
        case 's':
            lseek_offset = atoi(optarg);
            break;
        case 'S':
            if (strcmp(optarg, "SET") == 0) {
                lseek_whence = SEEK_SET;
            } else if (strcmp(optarg, "CUR") == 0) {
                lseek_whence = SEEK_CUR;
            } else if (strcmp(optarg, "END") == 0) {
                lseek_whence = SEEK_END;
            } else {
                fprintf(stderr, "Invalid lseek whence: %s\n", optarg);
                print_usage();
                return EXIT_FAILURE;
            }
            break;
        default:
            print_usage();
            return EXIT_FAILURE;
        }
    }

    const char *string_to_write = argv[optind];
    const char *filename = argv[optind + 1];

    int fd = open(filename, open_flags);
    if (fd < 0) {
        perror("open failed");
        return EXIT_FAILURE;
    }

    if (lseek(fd, lseek_offset, lseek_whence) < 0) {
        perror("lseek failed");
        close(fd);
        return EXIT_FAILURE;
    }

    ssize_t bytes_written = write(fd, string_to_write, strlen(string_to_write));
    if (bytes_written < 0) {
        perror("write failed");
        close(fd);
        return EXIT_FAILURE;
    }

    printf("write %zd byte(s) to %s\n", bytes_written, filename);

    close(fd);
    return EXIT_SUCCESS;
}
