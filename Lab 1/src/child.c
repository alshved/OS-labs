#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 2024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    char *filename = argv[1];
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("fopen");
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    while (fgets(buffer, BUFFER_SIZE, stdin)) {
        char *token = strtok(buffer, " ");
        if (token == NULL) {
            continue;
        }

        double first_num = atoi(token);
        int division_by_zero = 0;
        char result[BUFFER_SIZE];
        snprintf(result, BUFFER_SIZE, "%f", first_num); // Записываем первое число в результат

        token = strtok(NULL, " ");
        while (token != NULL) {
            int num = atoi(token);
            if (num == 0) {
                division_by_zero = 1;
                break;
            }
            first_num /= num;
            snprintf(result + strlen(result), BUFFER_SIZE - strlen(result), " / %d", num);
            token = strtok(NULL, " ");
        }

        if (division_by_zero) {
            fprintf(stderr, "Error: Division by zero detected. Terminating processes.\n");
            fclose(file);
            exit(1); // Завершаем дочерний процесс
        }

        fprintf(file, "Result: %f\n", first_num);
    }

    fclose(file);
    return 0;
}