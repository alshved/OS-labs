#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>

#define SHM_NAME "/shared_memory"
#define SEM_NAME "/semaphore"
#define SHM_SIZE 4096

int main()
{
    char filename[100];
    ssize_t bytes_read, bytes_written;
    const char *msg = "Введите имя файла: ";
    bytes_written = write(STDOUT_FILENO, msg, strlen(msg));
    if (bytes_written == -1)
    {
        const char error_msg[] = "error: failed to write to stdout\n";
        write(STDERR_FILENO, error_msg, sizeof(error_msg) - 1);
        exit(EXIT_FAILURE);
    }

    bytes_read = read(STDIN_FILENO, filename, sizeof(filename) - 1);
    if (bytes_read == -1)
    {
        const char error_msg[] = "error: failed to read from stdin\n";
        write(STDERR_FILENO, error_msg, sizeof(error_msg) - 1);
        exit(EXIT_FAILURE);
    }

    // Удаление символа новой строки, если он был введен
    if (bytes_read > 0 && filename[bytes_read - 1] == '\n')
    {
        filename[bytes_read - 1] = '\0';
    }
    else
    {
        filename[bytes_read] = '\0';
    }

    int shm_fd;
    void *shm_ptr;
    pid_t pid;
    char buffer[SHM_SIZE]; // Use an array instead of malloc for simplicity
    sem_t *sem;

    // Create shared memory
    sem_unlink(SEM_NAME);
    shm_unlink(SHM_NAME);

    shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        exit(1);
    }
    if (ftruncate(shm_fd, SHM_SIZE) == -1)
    {
        perror("ftruncate");
        shm_unlink(SHM_NAME);
        exit(1);
    }
    shm_ptr = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED)
    {
        perror("mmap");
        shm_unlink(SHM_NAME);
        close(shm_fd);
        exit(1);
    }

    sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 0);
    if (sem == SEM_FAILED)
    {
        perror("sem_open");
        munmap(shm_ptr, SHM_SIZE);
        shm_unlink(SHM_NAME);
        close(shm_fd);
        exit(1);
    }

    pid = fork();
    if (pid < 0)
    {
        perror("fork");
        exit(1);
    }
    else if (pid == 0)
    {
        execlp("./child", "child", filename, NULL);
        perror("execl");
        exit(1);
    }
    else
    {
        float num;
        int i = 0;
        while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0)
        {
            memcpy(shm_ptr, buffer, bytes_read);
            if (sem_post(sem) == -1)
            {
                perror("sem_post");
                exit(1);
            }

            memset(buffer, 0, sizeof(buffer));
        }

        if (bytes_read == -1)
        {
            perror("read");
            exit(1);
        }

        // Signal end of input to the child.
        memset(shm_ptr, 0, SHM_SIZE); // clear the shared memory before exiting
        if (sem_post(sem) == -1)
        {
            perror("sem_post");
            exit(1);
        }

        wait(NULL); // Wait for the child to finish

        munmap(shm_ptr, SHM_SIZE);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        sem_close(sem);
        sem_unlink(SEM_NAME);
        printf("Parent finished\n");
    }
    return 0;
}