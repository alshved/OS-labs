#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <ctype.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>

#define SHM_NAME "/shared_memory"
#define SEM_NAME "/semaphore"
#define SHM_SIZE 4096
#define MAX_NUMBERS 100

int convertStringToIntArray(const char *str, int array[], int *size)
{
    char *token;
    char *str_copy = strdup(str);
    *size = 0;
    token = strtok(str_copy, " ");
    while (token != NULL && *size < MAX_NUMBERS)
    {
        array[(*size)++] = atoi(token);
        token = strtok(NULL, " ");
    }
    free(str_copy);
    return *size;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        const char msg[] = "Usage: <program> <filename>\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }

    char *filename = argv[1];

    int file = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file == -1)
    {
        perror("open");
        exit(EXIT_FAILURE);
    } 

    int shm_fd;
    void *shm_ptr;
    sem_t *sem;
    char buffer[SHM_SIZE];

    shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        exit(1);
    }
    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
    {
        perror("mmap");
        close(shm_fd);
        exit(1);
    }
    sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED)
    {
        perror("sem_open");
        munmap(shm_ptr, SHM_SIZE);
        close(shm_fd);
        exit(1);
    }

    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        perror("sigaction");
        exit(1);
    }

    while (1)
    {
        if (sem_wait(sem) == -1)
        {
            if (errno == EINTR)
                continue;
            perror("sem_wait");
            munmap(shm_ptr, SHM_SIZE);
            close(shm_fd);
            exit(1);
        }

        memcpy(buffer, shm_ptr, SHM_SIZE);

        if (strlen(buffer) == 0)
            break; // check for empty buffer.

        if (buffer[0] == '\n')
        {
            kill(getppid(), SIGTERM);
            break;
        }
        int array[MAX_NUMBERS];
        int size = 0;
        convertStringToIntArray(buffer, array, &size);
        if (size < 2)
        {
            const char msg[] = "You have written few numbers\n";
            int32_t written = write(STDOUT_FILENO, msg, sizeof(msg));
            if (written != sizeof(msg))
            {
                const char msg[] = "error: failed to write to file\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            }
        }
        int divisible = array[0];
        for (int i = 1; i != size; ++i)
        {
            if (array[i] == 0)
            {
                kill(getppid(), SIGTERM);
                const char ermsg[] = "Division by zero\n";
                write(STDERR_FILENO, ermsg, sizeof(ermsg));
                exit(EXIT_FAILURE);
            }
            char msg[32];
            int32_t len = snprintf(msg, sizeof(msg) - 1, "%d : %d = %lf\n", divisible, array[i],
                                   ((float)divisible / array[i]));
            int32_t written = write(file, msg, len);
            if (written != len)
            {
                const char msg[] = "error: failed to write to file\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            }
        }
    }

    sem_close(sem);
    munmap(shm_ptr, SHM_SIZE);
    close(shm_fd);
    printf("Child finished\n");
    return 0;
}