#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

static char CLIENT_PROGRAM_NAME[] = "posix_ipc-client";

int main(int argc, char **argv) {
	if (argc == 1) {
		char msg[1024];
		uint32_t len = snprintf(msg, sizeof(msg) - 1, "usage: %s filename\n", argv[0]);
		write(STDERR_FILENO, msg, len);
		exit(EXIT_SUCCESS);
	}

	char progpath[1024];
	{
		ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1);
		if (len == -1) {
			const char msg[] = "error: failed to read full program path\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}
		while (progpath[len] != '/') --len;
		progpath[len] = '\0';
	}

	// Open pipe
	int channel[2];
	if (pipe(channel) == -1) {
		const char msg[] = "error: failed to create pipe\n";
		write(STDERR_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}
	const pid_t child = fork();

	switch (child) {
		case -1: {
			const char msg[] = "error: failed to spawn new process\n";
			write(STDERR_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		} break;

		case 0: {
			pid_t pid = getpid();

			dup2(STDIN_FILENO, channel[STDIN_FILENO]);
			close(channel[STDOUT_FILENO]);

			{
				char msg[64];
				const int32_t length = snprintf(msg, sizeof(msg), "%d: I'm a child\n", pid);
				write(STDOUT_FILENO, msg, length);
			}

			{
				char path[1024];
				snprintf(path, sizeof(path) - 1, "%s/%s", progpath, CLIENT_PROGRAM_NAME);
				char *const args[] = {CLIENT_PROGRAM_NAME, argv[1], NULL};

				int32_t status = execv(path, args);

				if (status == -1) {
					const char msg[] = "error: failed to exec into new exectuable image\n";
					write(STDERR_FILENO, msg, sizeof(msg));
					exit(EXIT_FAILURE);
				}
			}
		} break;

		default: {
			pid_t pid = getpid(); 
			{
				char msg[64];
				const int32_t length =
				    snprintf(msg, sizeof(msg), "%d: I'm a parent, my child has PID %d\n", pid, child);
				write(STDOUT_FILENO, msg, length);
			}
			int child_status;
			wait(&child_status);

			if (child_status != EXIT_SUCCESS) {
				const char msg[] = "error: child exited with error\n";
				write(STDERR_FILENO, msg, sizeof(msg));
				exit(child_status);
			}
		} break;
	}
}