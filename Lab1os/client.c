#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h> 
#include <sys/wait.h>

int main() {
    const char prompt[] = "Print filename: ";
    write(STDOUT_FILENO, prompt, sizeof(prompt) - 1);


    char filename[256];
    ssize_t n = read(STDIN_FILENO, filename, sizeof(filename) - 1);
    if (n <= 0) {
        const char err[] = "failed to read failename\n";
        write(STDERR_FILENO, err, sizeof(err) - 1);
        exit(1);
    }
    filename[n] = '\0';

    for (ssize_t i = 0; i < n; ++i) {
        if (filename[i] == '\n') {
            filename[i] = '\0';
            break;
        }
    }

    int file_fd = open(filename, O_RDONLY);
    if (file_fd == -1) {
        const char err[] = "failed to open file\n";
        write(STDERR_FILENO, err, sizeof(err) - 1);
        exit(1);
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        const char err[] = "failed to create pipe\n";
        write(STDERR_FILENO, err, sizeof(err) - 1);
        exit(1);
    }

    pid_t pid = fork();
    if (pid == -1) {
        const char err[] = "failed to spawn new process\n";
        write(STDERR_FILENO, err, sizeof(err) - 1);
        exit(1);
    }

    if (pid == 0) {

        dup2(file_fd, STDIN_FILENO);
        close(file_fd);

        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);

        char *args[] = {"./server", NULL};
        execv(args[0], args);

        const char err[] = "failed execv\n";
        write(STDERR_FILENO, err, sizeof(err) - 1);
        exit(1);

    } else {
        close(file_fd);
        close(pipefd[1]);

        char buf[4096];
        ssize_t bytes;
        while ((bytes = read(pipefd[0], buf, sizeof(buf))) > 0) {
            write(STDOUT_FILENO, buf, bytes);
        }
        close(pipefd[0]);

        int status;
        wait(&status);

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            const char err[] = "error in child\n";
            write(STDERR_FILENO, err, sizeof(err) - 1);
            return 1;
        }
    }

    return 0;
}