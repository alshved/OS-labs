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

    // Создание pipe1
    if (pipe(pipe1) == -1)
    {
        perror("pipe");
        exit(1);
    }
    printf("Введите имя файла: ");
    scanf("%s", filename);

    // Создание дочернего процесса
    pid = fork();

    if (pid == -1)
    {
        perror("fork");
        exit(1);
    }

    if (pid == 0)
    {
        // Дочерний процесс
        // Закрытие ненужных сторон pipe'ов
        close(pipe1[1]);

        // Перенаправление стандартного ввода на pipe1
        dup2(pipe1[0], STDIN_FILENO);
        close(pipe1[0]);

        // Запуск дочерней программы
        execlp("./child", "child", filename, NULL);
        perror("execlp");
        exit(1);
    }
    else
    {
        // Родительский процесс
        // Закрытие ненужных сторон pipe'ов
        close(pipe1[0]);
        ssize_t bytes;
        while (bytes = read(STDIN_FILENO, buffer, sizeof(buffer)))
        {

            if (bytes < 0)
            {
                const char msg[] = "error: failed to read from stdin\n";
                write(STDERR_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            }
            else if (buffer[0] == '\n')
            {
                break;
            }
            write(pipe1[1], buffer, strlen(buffer));
        }

        close(pipe1[1]);

        // Ожидание завершения дочернего процесса
        wait(NULL);
    }

    return 0;
}