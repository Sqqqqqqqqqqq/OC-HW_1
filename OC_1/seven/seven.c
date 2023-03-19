#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define BUF_SIZE 5000
#define FIFO1 "FIFO_1"
#define FIFO2 "FIFO_2"

char* find_last_sequence(const char* str, int n) {
    int len = strlen(str);
    char* last_sequence = NULL;

    for (int i = len - n; i >= 0; i--) {
        int j;
        for (j = 1; j < n; j++) {
            if (str[i + j] <= str[i + j - 1]) {
                break;
            }
        }
        if (j == n && !last_sequence) {
            last_sequence = (char*)malloc((n + 1) * sizeof(char));
            strncpy(last_sequence, str + i, n);
            last_sequence[n] = '\0';
        }
    }

    return last_sequence;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s input_file output_file N\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    unlink(FIFO1);
    unlink(FIFO2);

    if (mkfifo(FIFO1, 0666) == -1 || mkfifo(FIFO2, 0666) == -1) {
        perror("mkfifo");
        exit(1);
    }

    int n = atoi(argv[3]);
    int fd1, fd2;
    pid_t pid;

    // Create pipe
    fd1 = open(FIFO1, O_RDWR);
    fd2 = open(FIFO2, O_RDWR);

    if (fd1 == -1 || fd2 == -1) {
        perror("open");
        exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        char buf[BUF_SIZE];
        read(fd1, buf, BUF_SIZE);
        char* result = find_last_sequence(buf, n);
        write(fd2, result, sizeof(result));

        close(fd1);
        close(fd2);

        exit(EXIT_SUCCESS);
    } else {
        int input_fd = open(argv[1], O_RDONLY);
        char buf[BUF_SIZE];

        if (input_fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        ssize_t read_count = read(input_fd, buf, BUF_SIZE);
        if (read_count == -1) {
            perror("write");
            exit(EXIT_FAILURE);
        }
        write(fd1, buf, sizeof(buf));

        close(fd1);
        close(input_fd);

        int output_fd = open(argv[2], O_WRONLY | O_CREAT, 0644);
        if (output_fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        wait(NULL);
        ssize_t bytes_read;
        read(fd2, buf, BUF_SIZE);
        write(output_fd, buf, n);

        close(fd2);
        close(output_fd);

        unlink(FIFO1);
        unlink(FIFO2);

        exit(EXIT_SUCCESS);
    }
}

