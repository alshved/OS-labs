#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

#define BUFFER_SIZE 2048

int main()
{
    int pipe1[2];
    pid_t pid;
    char buffer[BUFFER_SIZE];
    char filename[100];
    ssize_t bytes_read, bytes_written;

    // Создание pipe1
    if (pipe(pipe1) == -1)
    {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // Сообщение пользователю для ввода имени файла
    const char *msg = "Введите имя файла: ";
    bytes_written = write(STDOUT_FILENO, msg, strlen(msg));
    if (bytes_written == -1)
    {
        const char error_msg[] = "error: failed to write to stdout\n";
        write(STDERR_FILENO, error_msg, sizeof(error_msg) - 1);
        exit(EXIT_FAILURE);
    }

    // Чтение имени файла с консоли
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

    // Создание дочернего процесса
    const pid_t child = fork();

    if (child == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (child == 0)
    {
        // Дочерний процесс
        pid_t pid = getpid();
        {
            char msg[64];
            const int32_t length = snprintf(msg, sizeof(msg), "%d: I'm a child\n", pid);
            write(STDOUT_FILENO, msg, length);
        }

        close(pipe1[1]);

        // Перенаправление стандартного ввода на pipe1
        if (dup2(pipe1[0], STDIN_FILENO) == -1)
        {
            perror("dup2");
            exit(1);
        }
        close(pipe1[0]);

        // Запуск дочерней программы
        execlp("./child", "child", filename, NULL);
        perror("execlp");
        exit(1);
    }
    else
    {
        // Родительский процесс
        // Закрытие ненужной стороны pipe'а
        close(pipe1[0]);

        pid_t pid = getpid(); 
			{
				char msg[64];
				const int32_t length =
				    snprintf(msg, sizeof(msg), "%d: I'm a parent, my child has PID %d\n", pid, child);
				write(STDOUT_FILENO, msg, length);
			}

        // Чтение данных с консоли и запись в pipe
        while ((bytes_read = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0)
        {
            if (buffer[0] == '\n')
            {
                break;
            }

            bytes_written = write(pipe1[1], buffer, bytes_read);
            if (bytes_written == -1)
            {
                const char error_msg[] = "error: failed to write to pipe\n";
                write(STDERR_FILENO, error_msg, sizeof(error_msg) - 1);
                exit(EXIT_FAILURE);
            }
        }

        if (bytes_read == -1)
        {
            const char error_msg[] = "error: failed to read from stdin\n";
            write(STDERR_FILENO, error_msg, sizeof(error_msg) - 1);
            exit(EXIT_FAILURE);
        }

        // Закрытие pipe'а после записи
        close(pipe1[1]);

        // Ожидание завершения дочернего процесса
        wait(NULL);
    }

    return 0;
}