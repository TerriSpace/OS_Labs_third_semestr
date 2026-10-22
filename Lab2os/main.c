#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

static int N;
static double *matrix;
static long long total_perms;
static long long next_perm = 0;
static double global_det = 0.0;

static pthread_mutex_t index_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t sum_mutex = PTHREAD_MUTEX_INITIALIZER;

static long long fact[13];

static long long factorial(int n) {
    long long f = 1;
    for (int i = 2; i <= n; i++) f *= i;
    return f;
}

static void get_kth_permutation(long long k, int n, int *perm) {
    int available[16];
    int avail_count = n;
    for (int i = 0; i < n; i++) available[i] = i;

    for (int i = 0; i < n; i++) {
        long long f = fact[n - 1 - i];
        int idx = (int)(k / f);
        perm[i] = available[idx];
        for (int j = idx; j < avail_count - 1; j++)
            available[j] = available[j + 1];
        avail_count--;
        k %= f;
    }
}

static int get_sign(const int *perm, int n) {
    int inv = 0;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            if (perm[i] > perm[j]) inv++;
    return (inv & 1) ? -1 : 1;
}

static void *worker(void *arg) {
    (void)arg;
    double local_sum = 0.0;
    int perm[16];

    while (1) {
        long long k;

        pthread_mutex_lock(&index_mutex);
        k = next_perm++;
        pthread_mutex_unlock(&index_mutex);

        if (k >= total_perms) break;

        get_kth_permutation(k, N, perm);
        int sign = get_sign(perm, N);

        double prod = 1.0;
        for (int i = 0; i < N; i++)
            prod *= matrix[i * N + perm[i]];

        local_sum += sign * prod;
    }

    pthread_mutex_lock(&sum_mutex);
    global_det += local_sum;
    pthread_mutex_unlock(&sum_mutex);

    return NULL;
}

int main(int argc, char **argv) {
    int num_threads = 4;
    N = 8;

    int opt;
    while ((opt = getopt(argc, argv, "n:t:h")) != -1) {
        switch (opt) {
            case 'n': N = atoi(optarg); break;
            case 't': num_threads = atoi(optarg); break;
            case 'h':
            default:
                fprintf(stderr,
                    "Usage: %s -n <matrix size> -t <threads>\n"
                    "  -n : size of matrix (1..12)\n"
                    "  -t : max number of threads\n",
                    argv[0]);
                return 1;
        }
    }

    if (N < 1 || N > 12) {
        fprintf(stderr, "Error: N must be in [1, 12].\n");
        return 1;
    }
    if (num_threads < 1) {
        fprintf(stderr, "Error: threads must be >= 1.\n");
        return 1;
    }

    for (int i = 0; i <= 12; i++) fact[i] = factorial(i);
    total_perms = fact[N];

    matrix = malloc((size_t)N * N * sizeof(double));
    if (!matrix) { perror("malloc"); return 1; }

    srand(67);
    for (int i = 0; i < N * N; i++)
        matrix[i] = (double)(rand() % 11 - 5);

    printf("Matrix %dx%d:\n", N, N);
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++)
            printf("%6.1f ", matrix[i * N + j]);
        printf("\n");
    }

    printf("\nConfiguration:\n");
    printf("  Matrix size       : %d\n", N);
    printf("  Threads requested : %d\n", num_threads);
    printf("  Total permutations: %lld\n\n", total_perms);

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    pthread_t *threads = malloc((size_t)num_threads * sizeof(pthread_t));
    if (!threads) { perror("malloc"); free(matrix); return 1; }

    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, worker, NULL) != 0) {
            perror("pthread_create");
            free(threads); free(matrix);
            return 1;
        }
    }

    for (int i = 0; i < num_threads; i++)
        pthread_join(threads[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &t_end);
    double elapsed = (t_end.tv_sec  - t_start.tv_sec) + (t_end.tv_nsec - t_start.tv_nsec) / 1e9;

    printf("Determinant = %.6f\n", global_det);
    printf("Time        = %.6f seconds\n", elapsed);

    free(threads);
    free(matrix);
    return 0;
}