// Compile: gcc -O2 main.c -o main
// Run: ./main < input.txt > output.txt

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>

#define NUM_MAPPERS 4
#define NUM_REDUCERS 2
#define MAX_LINE 1024
#define BUFFER_SIZE 4096

void error_exit(const char *msg) {
    perror(msg);
    exit(1);
}

unsigned int hash_word(const char *word, int num_reducers) {
    unsigned int hash = 0;
    for (int i = 0; word[i]; i++) {
        hash = hash * 31 + word[i];
    }
    return hash % num_reducers;
}

// Write all bytes to a file descriptor
ssize_t write_all(int fd, const char *buf, size_t count) {
    size_t bytes_written = 0;
    while (bytes_written < count) {
        ssize_t ret = write(fd, buf + bytes_written, count - bytes_written);
        if (ret < 0) {
            if (errno == EINTR) continue;
            return ret;
        }
        if (ret == 0) break;
        bytes_written += ret;
    }
    return bytes_written;
}

int main() {
    fprintf(stderr, "Starting main program\n");
    
    int mapper_stdin[NUM_MAPPERS][2];
    int mapper_stdout[NUM_MAPPERS][2];
    int reducer_stdin[NUM_REDUCERS][2];
    int reducer_stdout[NUM_REDUCERS][2];

    pid_t mapper_pids[NUM_MAPPERS], reducer_pids[NUM_REDUCERS];

    // Create pipes for mappers
    fprintf(stderr, "Creating mapper pipes\n");
    for (int i = 0; i < NUM_MAPPERS; i++) {
        if (pipe(mapper_stdin[i]) < 0 || pipe(mapper_stdout[i]) < 0)
            error_exit("pipe for mapper");
    }

    // Create pipes for reducers
    fprintf(stderr, "Creating reducer pipes\n");
    for (int i = 0; i < NUM_REDUCERS; i++) {
        if (pipe(reducer_stdin[i]) < 0 || pipe(reducer_stdout[i]) < 0)
            error_exit("pipe for reducer");
    }

    // Start mapper processes
    fprintf(stderr, "Starting mapper processes\n");
    for (int i = 0; i < NUM_MAPPERS; i++) {
        pid_t pid = fork();
        if (pid < 0) error_exit("fork mapper");

        if (pid == 0) { // Child process (mapper)
            fprintf(stderr, "Mapper %d starting\n", i);
            // Close unused pipe ends
            close(mapper_stdin[i][1]);
            close(mapper_stdout[i][0]);
            
            // Close all other pipes
            for (int j = 0; j < NUM_MAPPERS; j++) {
                if (j != i) {
                    close(mapper_stdin[j][0]);
                    close(mapper_stdin[j][1]);
                    close(mapper_stdout[j][0]);
                    close(mapper_stdout[j][1]);
                }
            }
            for (int j = 0; j < NUM_REDUCERS; j++) {
                close(reducer_stdin[j][0]);
                close(reducer_stdin[j][1]);
                close(reducer_stdout[j][0]);
                close(reducer_stdout[j][1]);
            }
            
            // Redirect stdin/stdout
            if (dup2(mapper_stdin[i][0], STDIN_FILENO) < 0) error_exit("dup2 stdin");
            if (dup2(mapper_stdout[i][1], STDOUT_FILENO) < 0) error_exit("dup2 stdout");
            
            // Close original pipe ends
            close(mapper_stdin[i][0]);
            close(mapper_stdout[i][1]);
            
            execlp("./mapper", "./mapper", NULL);
            error_exit("exec mapper");
        }
        
        mapper_pids[i] = pid;
    }

    // Start reducer processes
    fprintf(stderr, "Starting reducer processes\n");
    for (int i = 0; i < NUM_REDUCERS; i++) {
        pid_t pid = fork();
        if (pid < 0) error_exit("fork reducer");

        if (pid == 0) { // Child process (reducer)
            fprintf(stderr, "Reducer %d starting\n", i);
            // Close unused pipe ends
            close(reducer_stdin[i][1]);
            close(reducer_stdout[i][0]);
            
            // Close all other pipes
            for (int j = 0; j < NUM_MAPPERS; j++) {
                close(mapper_stdin[j][0]);
                close(mapper_stdin[j][1]);
                close(mapper_stdout[j][0]);
                close(mapper_stdout[j][1]);
            }
            for (int j = 0; j < NUM_REDUCERS; j++) {
                if (j != i) {
                    close(reducer_stdin[j][0]);
                    close(reducer_stdin[j][1]);
                    close(reducer_stdout[j][0]);
                    close(reducer_stdout[j][1]);
                }
            }
            
            // Redirect stdin/stdout
            if (dup2(reducer_stdin[i][0], STDIN_FILENO) < 0) error_exit("dup2 stdin");
            if (dup2(reducer_stdout[i][1], STDOUT_FILENO) < 0) error_exit("dup2 stdout");
            
            // Close original pipe ends
            close(reducer_stdin[i][0]);
            close(reducer_stdout[i][1]);
            
            execlp("./reducer", "./reducer", NULL);
            error_exit("exec reducer");
        }
        
        reducer_pids[i] = pid;
    }

    // Close unused pipe ends in parent
    for (int i = 0; i < NUM_MAPPERS; i++) {
        close(mapper_stdin[i][0]);
        close(mapper_stdout[i][1]);
    }
    for (int i = 0; i < NUM_REDUCERS; i++) {
        close(reducer_stdin[i][0]);
        close(reducer_stdout[i][1]);
    }

    // Distribute input to mappers
    fprintf(stderr, "Distributing input to mappers\n");
    char line[MAX_LINE];
    int current = 0;
    while (fgets(line, sizeof(line), stdin)) {
        fprintf(stderr, "Sending line to mapper %d: %s", current, line);
        write_all(mapper_stdin[current][1], line, strlen(line));
        current = (current + 1) % NUM_MAPPERS;
    }

    // Close mapper input pipes
    for (int i = 0; i < NUM_MAPPERS; i++) {
        close(mapper_stdin[i][1]);
    }

    // Process mapper output and send to reducers
    fprintf(stderr, "Processing mapper output\n");
    fd_set read_fds;
    int max_fd = -1;
    int active_mappers = NUM_MAPPERS;
    char buffer[BUFFER_SIZE];
    char word[256];
    int count;

    while (active_mappers > 0) {
        FD_ZERO(&read_fds);
        max_fd = -1;
        
        // Add active mapper output pipes to fd_set
        for (int i = 0; i < NUM_MAPPERS; i++) {
            if (mapper_stdout[i][0] != -1) {
                FD_SET(mapper_stdout[i][0], &read_fds);
                if (mapper_stdout[i][0] > max_fd) {
                    max_fd = mapper_stdout[i][0];
                }
            }
        }

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            error_exit("select");
        }

        for (int i = 0; i < NUM_MAPPERS; i++) {
            if (mapper_stdout[i][0] != -1 && FD_ISSET(mapper_stdout[i][0], &read_fds)) {
                ssize_t n = read(mapper_stdout[i][0], buffer, sizeof(buffer) - 1);
                if (n > 0) {
                    buffer[n] = '\0';
                    char *saveptr;
                    char *line = strtok_r(buffer, "\n", &saveptr);
                    
                    while (line != NULL) {
                        if (sscanf(line, "%255s %d", word, &count) == 2) {
                            fprintf(stderr, "Mapper %d output: %s %d\n", i, word, count);
                            int rid = hash_word(word, NUM_REDUCERS);
                            char outbuf[512];
                            int len = snprintf(outbuf, sizeof(outbuf), "%s %d\n", word, count);
                            write_all(reducer_stdin[rid][1], outbuf, len);
                        }
                        line = strtok_r(NULL, "\n", &saveptr);
                    }
                } else if (n == 0) {
                    close(mapper_stdout[i][0]);
                    mapper_stdout[i][0] = -1;
                    active_mappers--;
                }
            }
        }
    }

    // Close reducer input pipes
    for (int i = 0; i < NUM_REDUCERS; i++) {
        close(reducer_stdin[i][1]);
    }

    // Process reducer output
    fprintf(stderr, "Processing reducer output\n");
    int active_reducers = NUM_REDUCERS;
    max_fd = -1;

    while (active_reducers > 0) {
        FD_ZERO(&read_fds);
        max_fd = -1;
        
        // Add active reducer output pipes to fd_set
        for (int i = 0; i < NUM_REDUCERS; i++) {
            if (reducer_stdout[i][0] != -1) {
                FD_SET(reducer_stdout[i][0], &read_fds);
                if (reducer_stdout[i][0] > max_fd) {
                    max_fd = reducer_stdout[i][0];
                }
            }
        }

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            error_exit("select");
        }

        for (int i = 0; i < NUM_REDUCERS; i++) {
            if (reducer_stdout[i][0] != -1 && FD_ISSET(reducer_stdout[i][0], &read_fds)) {
                ssize_t n = read(reducer_stdout[i][0], buffer, sizeof(buffer));
                if (n > 0) {
                    write_all(STDOUT_FILENO, buffer, n);
                } else if (n == 0) {
                    close(reducer_stdout[i][0]);
                    reducer_stdout[i][0] = -1;
                    active_reducers--;
                }
            }
        }
    }

    // Wait for all child processes
    fprintf(stderr, "Waiting for child processes\n");
    for (int i = 0; i < NUM_MAPPERS; i++) {
        waitpid(mapper_pids[i], NULL, 0);
    }
    for (int i = 0; i < NUM_REDUCERS; i++) {
        waitpid(reducer_pids[i], NULL, 0);
    }

    fprintf(stderr, "Program completed\n");
    return 0;
}

