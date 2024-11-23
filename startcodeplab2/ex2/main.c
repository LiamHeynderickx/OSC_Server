#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 1024


void reverse_case(char *message) {
    for (int i = 0; message[i] != '\0'; i++) {
        if (islower(message[i])) {
            message[i] = toupper(message[i]);
        } else if (isupper(message[i])) {
            message[i] = tolower(message[i]);
        }
    }
}

int main() {
    int pipe_fd[2];
    pid_t pid;
    char message[BUFFER_SIZE] = "Hi There";  // Parent's message
    char buffer[BUFFER_SIZE];

    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    }
    if (pid > 0) { // Parent process
        // Close the read end of the pipe
        close(pipe_fd[0]);

        // Write the message to the pipe
        write(pipe_fd[1], message, strlen(message) + 1);
        close(pipe_fd[1]); // Close the write end of the pipe
    }

    if (pid == 0) { // Child process
        close(pipe_fd[1]);
        read(pipe_fd[0], buffer, BUFFER_SIZE);
        close(pipe_fd[0]);
        reverse_case(buffer);

        // Print the modified message
        printf("Child Process Output: %s\n", buffer);
    }

    return 0;
}
