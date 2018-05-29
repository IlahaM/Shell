#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>

int id = 0;
char *position;
char *command;
char buf[256];
int i = 0;
int i2 = 0;
int i_and = 0;
int i_back = 0;
int i_pipe = 0;
int i_a2 = 0;
char *c, *c_pipe, *c_and, *c_a2, *c_redOut, *c_ro2, *p, *c_back, *c_p;
char **arguments_pipe;
char **arguments_and;
char **arguments_back;
char **args_withinpipe;
char **args_withinBack;
char **args_withinAnd;
char **args_redOut;
char **args_withinRedOut;
char **args_withinRedOut2;
char **args_withinRedOut_Err;
char dir[1000];
int flag = 0;
int flag_a = 0;
int flag_back = 0;
int std_err = 0;
int first = 0;
int last = 0;
int redirect = 0;

void sig_handler(int signo) {
	if ((signo == SIGINT)) {
		signal(SIGINT, SIG_IGN);
		kill(0, SIGINT);
	} /*else if (signo == SIGSTOP) {
	 signal(SIGSTOP, SIG_IGN);
	 kill(0, SIGSTOP);
	 }*/
}

void cmdPipe(char* command) {

	signal(SIGINT, SIG_IGN);
	//signal(SIGSTOP, SIG_IGN);

	char *pos;
	char *cmd = calloc(100, sizeof(char));

	if (redirect) {
		int i_redOut = 0;
		char *ch;
		char **args;
		char *f;
		char **file;
		int num = 0;
		int fd_out = 0;
		int fd_out_a = 0;
		int fd_in = 0;

		if ((flag) && !(flag_a)) {
			ch = strtok(command, ">");
			args = calloc(100, sizeof(char*));
		} else if (flag && flag_a) {
			ch = strtok(command, ">>");
			args = calloc(100, sizeof(char*));
		} else if (!(flag)) {
			ch = strtok(command, "<");
			args = calloc(100, sizeof(char*));
		}

		while (ch != NULL) {
			if ((flag) && !(flag_a)) {
				args[i_redOut] = ch;
				i_redOut++;
				ch = strtok(NULL, ">");
			} else if (flag && flag_a) {
				args[i_redOut] = ch;
				i_redOut++;
				ch = strtok(NULL, ">>");
			} else if (!(flag)) {
				args[i_redOut] = ch;
				i_redOut++;
				ch = strtok(NULL, "<");
			}
		}

		if (!std_err) {
			strcpy(cmd, args[0]);
		} else {
			if ((pos = strchr(args[0], '2')) != NULL) {
				*pos = '\0';
			}
			strcpy(cmd, args[0]);
		}

		f = strtok(args[1], " ");
		file = calloc(100, sizeof(char*));
		while (f != NULL) {
			file[num] = f;
			num++;
			f = strtok(NULL, " ");
		}

		if ((flag) && !(flag_a)) {
			fd_out = open(file[0], O_RDWR | O_CREAT | O_TRUNC, 0777);
			if (fd_out < 0) {
				fprintf(stderr, "error in opening file writing\n");
			}
			if (std_err == 0) {
				dup2(fd_out, STDOUT_FILENO);
				close(fd_out);
			} else if (std_err) {
				dup2(fd_out, STDERR_FILENO);
				close(fd_out);
			}
		}

		else if (flag && flag_a) {
			fd_out_a = open(file[0], O_RDWR | O_CREAT | O_APPEND, 0777);
			if (fd_out_a < 0) {
				fprintf(stderr, "error in opening file writing\n");
			}
			if (std_err == 0) {
				dup2(fd_out_a, STDOUT_FILENO);
				close(fd_out_a);
			} else if (std_err) {
				dup2(fd_out_a, STDERR_FILENO);
				close(fd_out_a);
			}
		}

		else if (!(flag)) {
			fd_in = open(file[0], O_RDONLY, 0777);
			if (fd_in < 0) {
				fprintf(stderr, "error in opening file reading\n");
			}
			dup2(fd_in, STDIN_FILENO);
			close(fd_in);
		}

	} else {
		strcpy(cmd, command);
	}

	int k = 0;
	int i_pipe = 0;
	int status;
	pid_t pid;
	int j = 0;

	c_pipe = strtok(cmd, "|");
	arguments_pipe = calloc(100, sizeof(char*));
	while (c_pipe != NULL) {
		arguments_pipe[i_pipe] = c_pipe;
		i_pipe++;
		c_pipe = strtok(NULL, "|");
	}

	int numPipes = i_pipe - 1;
	int fd[2 * numPipes];

	for (int i = 0; i < numPipes; i++) {
		if (pipe(fd + i * 2) < 0) {
			fprintf(stderr, "pipe error\n");
			exit(EXIT_FAILURE);
		}
	}

	for (int h = 0; h < i_pipe; h++) {
		if (h == 0) {
			first = 1;
		} else {
			first = 0;
		}
		if (h == (i_pipe - 1)) {
			last = 1;
		} else {
			last = 0;
		}

		c_p = strtok(arguments_pipe[h], " ");
		args_withinpipe = calloc(100, sizeof(char*));
		while (c_p != NULL) {
			args_withinpipe[k] = c_p;
			k++;
			c_p = strtok(NULL, " ");
		}

		pid = fork();
		if (pid == 0) {

			signal(SIGINT, SIG_DFL);
			//signal(SIGSTOP, SIG_DFL);

			if (!last) {
				if (dup2(fd[j + 1], 1) < 0) {
					fprintf(stderr, "dup2 error\n");
					exit(EXIT_FAILURE);
				}
			}
			if (j != 0) {
				if (dup2(fd[j - 2], 0) < 0) {
					fprintf(stderr, "dup2 error\n");
					exit(EXIT_FAILURE);
				}
			}

			for (i = 0; i < 2 * numPipes; i++) {
				close(fd[i]);
			}

			if (execvp(args_withinpipe[0], args_withinpipe) < 0) {
				fprintf(stderr, "exec error\n");
				exit(EXIT_FAILURE);
			}
		} else if (pid < 0) {
			fprintf(stderr, "error\n");
			exit(EXIT_FAILURE);
		}
		j += 2;
		k = 0;
	}
	i_pipe = 0;

	signal(SIGINT, sig_handler);
	//signal(SIGSTOP, sig_handler);
	for (i = 0; i < 2 * numPipes; i++) {
		close(fd[i]);
	}

	for (i = 0; i < numPipes + 1; i++) {
		wait(&status);
	}
}

void changeDir(char* command) {

	char** args_dir;
	char* d;
	int t = 0;

	d = strtok(command, " ");
	args_dir = calloc(100, sizeof(char*));
	while (d != NULL) {
		args_dir[t] = d;
		t++;
		d = strtok(NULL, " ");
	}

	getcwd(dir, sizeof(dir));
	printf("current dir: %s\n", dir);

	//char* ch = (char*) command + 3;
	//chdir(ch);
	if (chdir(args_dir[t - 1]) < 0) {
		fprintf(stderr, "no such file or directory\n");
	} else {
		getcwd(dir, sizeof(dir));
		printf("changed dir: %s\n", dir);
	}

}

void execCmd(char* command) {

	int i = 0;
	char **arguments;
	int status;

	signal(SIGINT, SIG_IGN);
	//signal(SIGSTOP, SIG_IGN);

	int id = fork();

	if (id == 0) {

		signal(SIGINT, SIG_DFL);
		//signal(SIGSTOP, SIG_DFL);
		if (flag_back) {
			printf("child executing in background\n");
			setpgid(0, 0);
		}

		c = strtok(command, " ");
		arguments = calloc(100, sizeof(char*));
		while (c != NULL) {
			arguments[i] = c;
			i++;
			c = strtok(NULL, " ");
		}

		if (execvp(arguments[0], arguments) < 0) {
			fprintf(stderr, "exec error\n");
		}
		free(arguments);

	} else {

		signal(SIGINT, sig_handler);
		//signal(SIGSTOP, sig_handler);

		if (!flag_back) {
			wait(NULL);
		} else if (flag_back) {
			printf("parent not being blocked\n");
			waitpid(id, &status, WNOHANG);
		}
	}
	return;
}

void red_in_out(char* command) {

	signal(SIGINT, SIG_IGN);
	//signal(SIGSTOP, SIG_IGN);

	// flag = 0 -> in_dir; flag = 1 -> out_dir; flag_a = 0 -> trunc; flag_a = 1 -> append
	int i_redOut = 0;
	int k = 0;
	int j = 0;
	int fd_out = 0;
	int fd_out_a = 0;
	int fd_in = 0;

	if (fork() == 0) {

		signal(SIGINT, SIG_DFL);
		//signal(SIGSTOP, SIG_DFL);

		if ((flag) && !(flag_a)) {
			c_redOut = strtok(command, ">");
			args_redOut = calloc(100, sizeof(char*));
		} else if (flag && flag_a) {
			c_redOut = strtok(command, ">>");
			args_redOut = calloc(100, sizeof(char*));
		} else if (!(flag)) {
			c_redOut = strtok(command, "<");
			args_redOut = calloc(100, sizeof(char*));
		}

		while (c_redOut != NULL) {
			if ((flag) && !(flag_a)) {
				args_redOut[i_redOut] = c_redOut;
				i_redOut++;
				c_redOut = strtok(NULL, ">");
			} else if (flag && flag_a) {
				args_redOut[i_redOut] = c_redOut;
				i_redOut++;
				c_redOut = strtok(NULL, ">>");
			} else if (!(flag)) {
				args_redOut[i_redOut] = c_redOut;
				i_redOut++;
				c_redOut = strtok(NULL, "<");
			}
		}

		c_ro2 = strtok(args_redOut[0], " ");
		args_withinRedOut = calloc(100, sizeof(char*));
		while (c_ro2 != NULL) {
			args_withinRedOut[k] = c_ro2;
			k++;
			c_ro2 = strtok(NULL, " ");
		}

		c_ro2 = strtok(args_redOut[1], " ");
		args_withinRedOut2 = calloc(100, sizeof(char*));

		while (c_ro2 != NULL) {
			args_withinRedOut2[j] = c_ro2;
			j++;
			c_ro2 = strtok(NULL, " ");
		}

		if ((flag) && !(flag_a)) {
			fd_out = open(args_withinRedOut2[0], O_RDWR | O_CREAT | O_TRUNC,
					0777);
			if (fd_out < 0) {
				fprintf(stderr, "error in opening file writing\n");
			}
			if (std_err == 0) {
				dup2(fd_out, STDOUT_FILENO);
				close(fd_out);
			} else if (std_err) {
				dup2(fd_out, STDERR_FILENO);
				close(fd_out);
			}
		}

		else if (flag && flag_a) {
			fd_out_a = open(args_withinRedOut2[0], O_RDWR | O_CREAT | O_APPEND,
					0777);
			if (fd_out_a < 0) {
				fprintf(stderr, "error in opening file writing\n");
			}
			if (std_err == 0) {
				dup2(fd_out_a, STDOUT_FILENO);
				close(fd_out_a);
			} else if (std_err) {
				dup2(fd_out_a, STDERR_FILENO);
				close(fd_out_a);
			}
		}

		else if (!(flag)) {
			fd_in = open(args_withinRedOut2[0], O_RDONLY, 0777);
			if (fd_in < 0) {
				fprintf(stderr, "error in opening file reading\n");
			}
			dup2(fd_in, STDIN_FILENO);
			close(fd_in);
		}

		args_withinRedOut_Err = calloc(100, sizeof(char*));

		for (int s = 0; s < k - 1; s++) {
			args_withinRedOut_Err[s] = args_withinRedOut[s];
		}

		if (std_err) {
			if (execvp(args_withinRedOut[0], args_withinRedOut_Err) < 0) {
				fprintf(stderr, "error exec\n");
			}
		} else {
			if (execvp(args_withinRedOut[0], args_withinRedOut) < 0) {
				fprintf(stderr, "error exec\n");
			}
		}

	} else {
		signal(SIGINT, sig_handler);
		//signal(SIGSTOP, sig_handler);
		wait(0);
	}
}

int main(int argc, char **argv) {

	signal(SIGINT, SIG_IGN);
	//signal(SIGSTOP, SIG_IGN);

	command = calloc(100, sizeof(char));

	while (1) {

		fgets(buf, sizeof(buf), stdin);

		if ((strchr(buf, '\\')) != NULL) {

			if ((position = strchr(buf, '\n')) != NULL) {
				*position = '\0';
			}
			if ((position = strchr(buf, '\\')) != NULL) {
				*position = '\0';
			}
			command = realloc(command, strlen(command) + 1 + strlen(buf));
			strcat(command, buf);
			continue;
		}

		if ((position = strchr(buf, '\n')) != NULL) {
			*position = '\0';
		}
		strcat(command, buf);

		// flag = 0 -> in_dir; flag = 1 -> out_dir; flag_a = 0 -> trunc; flag_a = 1 -> append
		if ((strstr(command, ">") != NULL) || (strstr(command, ">>") != NULL)
				|| (strstr(command, "<") != NULL)) {

			if (strstr(command, ">>") != NULL) {
				flag = 1;
				flag_a = 1;
			}
			if (strstr(command, ">") != NULL) {
				flag = 1;
			}
			if (strstr(command, "2") != NULL) {
				std_err = 1;
			}
		}

		if (strstr(command, "|") != NULL) {
			if (flag || flag_a || std_err)
				redirect = 1;
			cmdPipe(command);
			redirect = 0;
			flag = 0;
			flag_a = 0;
			std_err = 0;
		}

		else if (strstr(command, "&&") != NULL) {
			if (flag || flag_a || std_err)
				redirect = 1;
			c_and = strtok(command, "&&");
			arguments_and = calloc(100, sizeof(char*));

			while (c_and != NULL) {
				arguments_and[i_and] = c_and;
				i_and++;
				c_and = strtok(NULL, "&&");
			}

			int index = i_and - 1;

			if (redirect) {
				for (int k = 0; k < index; k++) {
					execCmd(arguments_and[k]);
				}
				red_in_out(arguments_and[index]);
			} else {
				for (int k = 0; k < i_and; k++) {
					execCmd(arguments_and[k]);
				}
			}
			redirect = 0;
			flag = 0;
			flag_a = 0;
			std_err = 0;
		}

		else if (strstr(command, "&") != NULL) {
			flag_back = 1;
			c_back = strtok(command, "&");
			arguments_back = calloc(100, sizeof(char*));

			while (c_back != NULL) {
				arguments_back[i_back] = c_back;
				i_back++;
				c_back = strtok(NULL, "&");
			}
			execCmd(arguments_back[0]);
			flag_back = 0;
		}

		else if (strstr(command, "cd") != NULL) {
			changeDir(command);
		}

		else if ((strstr(command, "ls") != NULL)
				|| (strstr(command, "cp") != NULL)
				|| (strstr(command, "grep") != NULL)
				|| (strstr(command, "find") != NULL)
				|| (strstr(command, "sort") != NULL)
				|| (strstr(command, "wc") != NULL)
				|| (strstr(command, "./") != NULL)) {

			if (flag || flag_a || std_err)
				redirect = 1;
			if (redirect) {
				red_in_out(command);
			} else {
				execCmd(command);
			}
			redirect = 0;
			flag = 0;
			flag_a = 0;
			std_err = 0;
		}

		command = calloc(100, sizeof(char));
		i_and = 0;
		continue;
	}
	exit(0);
}
