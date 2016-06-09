#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#define MAX_WORDS 16
#define MAX_LINE 80

int main() {
    char c;
    char words[MAX_WORDS][MAX_LINE];
    int word_length, word_count;
    int first_exec_name, first_args_begin, first_args_end;
    int first_input_file, first_output_file;
    char* first_args[MAX_WORDS];
    int second_exec_name, second_args_begin, second_args_end;
    int second_input_file, second_output_file;
    char* second_args[MAX_WORDS];
    int is_parsing_first = 1;
    int i;

    printf("$");

    // info for first execution
    first_exec_name = 0;
    first_args_begin = 0;
    first_input_file = -1;
    first_output_file = -1;
    first_args_end = -1;

    // info for second execution
    second_exec_name = -1;
    second_args_begin = -1;
    second_input_file = -1;
    second_output_file = -1;
    second_args_end = -1;

    // length of current parsing word
    word_length = 0;
    // current amount of words in buffer
    word_count = 0;

    c = getchar();
    while (c != EOF) {
        if (c == ' ' || c == '\n' || c == '>' || c == '<' || c == '|') {
            words[word_count][word_length] = '\0';
            if (word_length > 0) {
                word_count = word_count + 1;
            }
            word_length = 0;
            if (c == '>') {
                if (is_parsing_first == 1) {
                    first_output_file = word_count;
                    if (first_args_end < 0) {
                        first_args_end = word_count;
                    }
                } else {
                    second_output_file = word_count;
                    if (second_args_end < 0) {
                        second_args_end = word_count;
                    }
                  }
            }
            if (c == '<') {
                if (is_parsing_first == 1) {
                    if (first_args_end < 0) {
                        first_args_end = word_count;
                    }
                    first_input_file = word_count;
                } else {
                    second_input_file = word_count;
                    if (second_args_end < 0) {
                        second_args_end = word_count;
                    }
                  }
            }
            if (c == '|') {
                is_parsing_first = 0;
                if (first_args_end < 0) {
                    first_args_end = word_count;
                }
                second_exec_name = word_count;
                second_args_begin = word_count;
            }
            if (c == '\n') {
                break;
            }
            c = getchar();
            while (c == ' ') {
                c = getchar();
            }
        } else {
            words[word_count][word_length] = c;
            word_length = word_length + 1;
            c = getchar();
        }
    }
    if (is_parsing_first == 1) {
        if (first_args_end < 0) {
            first_args_end = word_count;
        }
        for (i = first_args_begin; i < first_args_end; i++) {
            first_args[i - first_args_begin] = words[i];
        }
        first_args[first_args_end - first_args_begin] = NULL;
        pid_t pid = fork();
        if (!pid) {
            // child process          
            if (first_output_file > 0) {
                int fd = open(words[first_output_file],
                          O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (fd == -1) {
                    perror("open");
                    return EXIT_FAILURE;
                }
                if (-1 == dup2(fd, STDOUT_FILENO)) {
                    perror("dup2");
                    return EXIT_FAILURE;
                }
            }
            if (first_input_file > 0) {
                int fd = open(words[first_input_file], O_RDONLY);

                if (fd == -1) {
                    perror("open");
                    return EXIT_FAILURE;
                }
                if (-1 == dup2(fd, STDIN_FILENO)) {
                    perror("dup2");
                    return EXIT_FAILURE;
                }
            }
            int rv = execvp(first_args[0], first_args);
            if (rv == -1) {
                perror("execvp");
                return EXIT_FAILURE;
            }
        }
        // parent branch
        pid = wait(NULL);
        if (pid == -1) {
            perror("wait");
            return EXIT_FAILURE;
        }
        printf("$");
    } else {
        // got two files to exec      
        // preparing first arguments
        if (first_args_end < 0) {
            first_args_end = word_count;
        }
        for (i = first_args_begin; i < first_args_end; i++) {
            first_args[i - first_args_begin] = words[i];
        }
        first_args[first_args_end - first_args_begin] = NULL;
        // preparing second arguments
        if (second_args_end < 0) {
            second_args_end = word_count;
        }
        for (i = second_args_begin; i < second_args_end; i++) {
            second_args[i - second_args_begin] = words[i];
        }
        second_args[second_args_end - second_args_begin] = NULL;

        // creating descriptors for pipe between processes
        int fds[2];
        if (pipe(fds) != 0) {
            perror("pipe");
            return EXIT_FAILURE;
        }
        pid_t pid = fork();
        if (!pid) {
            // first child process          
            close(fds[0]);
            if (-1 == dup2(fds[1], STDOUT_FILENO)) {
                perror("dup2");
                return EXIT_FAILURE;
            }
            if (first_input_file > 0) {
                int fd = open(words[first_input_file], O_RDONLY);

                if (fd == -1) {
                    perror("open");
                    return EXIT_FAILURE;
                }
                if (-1 == dup2(fd, STDIN_FILENO)) {
                    perror("dup2");
                    return EXIT_FAILURE;
                }
            }
            int rv = execvp(first_args[0], first_args);
            if (rv == -1) {
                perror("execvp");
                return EXIT_FAILURE;
            }
        }
        pid_t pid2 = fork();
        if (!pid2) {
            // second child process
            close(fds[1]);
            if (-1 == dup2(fds[0], STDIN_FILENO)) {
                perror("dup2");
                return EXIT_FAILURE;
            }
            if (second_output_file > 0) {
                int fd = open(words[second_output_file],
                          O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (fd == -1) {
                    perror("open");
                    return EXIT_FAILURE;
                }
                if (-1 == dup2(fd, STDOUT_FILENO)) {
                    perror("dup2");
                    return EXIT_FAILURE;
                }
            }
            int rv = execvp(second_args[0], second_args);
            if (rv == -1) {
                perror("execvp");
                return EXIT_FAILURE;
            }
        }
        close(fds[1]);

        // parent branch
        pid = wait(NULL);
        if (pid == -1) {
            perror("wait");
            return EXIT_FAILURE;
        }
        // parent branch
        pid2 = wait(NULL);
        if (pid2 == -1) {
            perror("wait");
            return EXIT_FAILURE;
        }
        close(fds[0]);
        printf("$");
    }
    printf("\n");
    return EXIT_SUCCESS;
}
