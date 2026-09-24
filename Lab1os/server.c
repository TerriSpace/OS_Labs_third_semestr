#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static ssize_t read_line(int fd, char *buf, size_t max) {
    size_t i = 0;
    while (i < max - 1) {
        char c;
        ssize_t n = read(fd, &c, 1);
        if (n <= 0) {
            if (i == 0) return n;
            break;
        }
        if (c == '\n') break;
        buf[i++] = c;
    }
    buf[i] = '\0';
    return (ssize_t)i;
}

static int int_to_str(int val, char *buf) {
    char tmp[16];
    int i = 0;
    int neg = 0;
    unsigned int uval;

    if (val < 0) {
        neg = 1;
        uval = (unsigned int)(-(long)val);
    } else {
        uval = (unsigned int)val;
    }

    if (uval == 0) tmp[i++] = '0';
    while (uval) {
        tmp[i++] = '0' + (uval % 10);
        uval /= 10;
    }

    int j = 0;
    if (neg) buf[j++] = '-';
    while (i > 0) buf[j++] = tmp[--i];
    return j;
}

int main() {
    char line[4096];

    while (1) {
        ssize_t n = read_line(STDIN_FILENO, line, sizeof(line));
        if (n <= 0) break;   // EOF

        char *token = strtok(line, " \t");
        if (token == NULL) continue;

        int result = atoi(token);
        token = strtok(NULL, " \t");

        while (token != NULL) {
            int divisor = atoi(token);

            if (divisor == 0) {
                const char err[] = "error - division by zero\n";
                write(STDERR_FILENO, err, sizeof(err) - 1);
                exit(1);
            }

            result /= divisor;
            token = strtok(NULL, " \t");
        }

        char out[32];
        int len = int_to_str(result, out);
        out[len++] = '\n';
        write(STDOUT_FILENO, out, len);
    }

    return 0;
}