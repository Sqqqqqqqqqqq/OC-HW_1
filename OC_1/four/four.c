#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUF_SIZE 5000

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

    int n = atoi(argv[3]);
    int fd1[2], fd2[2];
    pid_t pid1, pid2, pid3;

    // Create pipe
    if (pipe(fd1) == -1 || pipe(fd2) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    //1st process
    pid1 = fork();
    if (pid1 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid1 == 0) {
        // child process
        close(fd1[0]);
        close(fd2[0]);

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
        write(fd1[1], buf, sizeof(buf));

        close(fd1[1]);
        close(fd2[1]);
        close(input_fd);

        exit(EXIT_SUCCESS);
    }

    pid2 = fork();
    if (pid2 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid2 == 0) {    // дочерний
        close(fd1[1]);  // Close unused write end of first pipe
        close(fd2[0]);  // Close unused read end of second pipe

        char buf[BUF_SIZE];
        read(fd1[0], buf, BUF_SIZE);
        char* result = find_last_sequence(buf, n);
        write(fd2[1], result, sizeof(result));

        close(fd1[0]);
        close(fd2[1]);

        exit(EXIT_SUCCESS);
    }

    // 3rd process
    pid3 = fork();
    if (pid3 == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid3 == 0) {
        close(fd1[1]);
        close(fd2[1]);

        int output_fd = open(argv[2], O_WRONLY | O_CREAT, 0644);
        if (output_fd == -1) {
            perror("open");
            exit(EXIT_FAILURE);
        }

        char buf[BUF_SIZE];
        ssize_t bytes_read;
        while ((bytes_read = read(fd2[0], buf, BUF_SIZE)) > 0) {
            write(output_fd, buf, n);
        }

        close(fd1[0]);
        close(fd2[0]);
        close(output_fd);

        exit(EXIT_SUCCESS);
    }

    exit(0);
}
