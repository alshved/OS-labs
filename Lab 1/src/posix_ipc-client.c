#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

int MAX_LENGTH = 1024;
int MAX_NUMBERS = 100;

void convertStringToIntArray(const char *str, int intArray[], int *size) {
	int i = 0, num = 0;
	*size = 0;
	while (str[i] != '\n') {
		while (str[i] != '\n' && isspace(str[i])) {
			i++;
		}
		if (str[i] == '\n') {
			break;
		}
		num = 0;
		while (str[i] != '\n' && isdigit(str[i])) {
			num = num * 10 + (str[i] - '0');
			i++;
		}
		if (*size < MAX_NUMBERS) {
			intArray[*size] = num;
			(*size)++;
		}
	}
}

int main(int argc, char **argv) {
	char buf[4096];
	ssize_t bytes;

	pid_t pid = getpid();

	int32_t file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0600);
	if (file == -1) {
		const char msg[] = "error: failed to open requested file\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	{
		char msg[128];
		int32_t len =
		    snprintf(msg, sizeof(msg) - 1,
		             "%d: Start typing lines of text. Press 'Ctrl-D' or 'Enter' with no input to exit\n", pid);
		write(STDOUT_FILENO, msg, len);
	}

	while (bytes = read(STDIN_FILENO, buf, sizeof(buf))) {
		if (bytes < 0) {
			const char msg[] = "error: failed to read from stdin\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		} else if (buf[0] == '\n') {
			break;
		}

		{
			int array[MAX_NUMBERS];
			int size = 0;
			convertStringToIntArray(buf, array, &size);
			if (size < 2) {
				const char msg[] = "You have written few numbers\n";
				int32_t written = write(STDOUT_FILENO, msg, sizeof(msg));
				if (written != sizeof(msg)) {
					const char msg[] = "error: failed to write to file\n";
					write(STDERR_FILENO, msg, sizeof(msg));
					exit(EXIT_FAILURE);
				}
			}
			int divisible = array[0];
			for (int i = 1; i != size; ++i) {
				if (array[i] == 0) {
					const char ermsg[] = "Division by zero\n";
					write(STDERR_FILENO, ermsg, sizeof(ermsg));
					exit(EXIT_FAILURE);
				}
				char msg[32];
				int32_t len = snprintf(msg, sizeof(msg) - 1, "%d : %d = %lf\n", divisible, array[i],
				                       ((float)divisible / array[i]));
				int32_t written = write(file, msg, len);
				if (written != len) {
					const char msg[] = "error: failed to write to file\n";
					write(STDERR_FILENO, msg, sizeof(msg));
					exit(EXIT_FAILURE);
				}
			}
		}
	}

	const char term = '\0';
	write(file, &term, sizeof(term));

	close(file);
}