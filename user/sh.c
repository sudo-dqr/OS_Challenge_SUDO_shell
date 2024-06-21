#include <args.h>
#include <lib.h>

#define WHITESPACE " \t\r\n"
#define SYMBOLS "<|>&;()"

int flag = 0;
int job_flag = 0; // 遇到后台指令

/* Overview:
 *   Parse the next token from the string at s.
 *
 * Post-Condition:
 *   Set '*p1' to the beginning of the token and '*p2' to just past the token.
 *   Return:
 *     - 0 if the end of string is reached.
 *     - '<' for < (stdin redirection).
 *     - '>' for > (stdout redirection).
 *     - '|' for | (pipe).
 *     - 'w' for a word (command, argument, or file name).
 *
 *   The buffer is modified to turn the spaces after words into zero bytes ('\0'), so that the
 *   returned token is a null-terminated string.
 */
int _gettoken(char *s, char **p1, char **p2) {
	*p1 = 0;
	*p2 = 0;
	if (s == 0) {
		return 0;
	}

	while (strchr(WHITESPACE, *s)) {
		*s++ = 0;
	}

	/*Shell Challenge : "content"*/
	if (*s == '"') { // read until the next '"'
		*s++;
		*p1 = s;
		while (*s && *s != '"') {
			s++;
		}
		*(s++) = 0; // *s = '"'
		*p2 = s;
		return 'w';
	}
	
	if(*s == '`') { // 识别反引号
    	*s = 0;
    	s++;
    	*p1 = s;
   		while(*s && (*s != '`')){
        	s++;
    	}
    	*s++ = 0;
    	*p2 = s;
    	return 'f'; 
	}

	if (*s == 0) {
		return 0;
	}

	if (strchr(SYMBOLS, *s)) { // 修改此处逻辑 识别 && || ;
		int t = *s;
		*p1 = s;
		/*Shell Challenge*/
		char *s1 = s + 1;
		if (*s1 == *s && *s1 == '&') {
			s++;
			t = 'a';
		} else if (*s1 == *s && *s1 == '|') {
			s++;
			t = 'o';
		} else if (*s1 == *s && *s1 == '>') {
			s++;
			t = 'z';
		}
		*s++ = 0;
		*p2 = s;
		return t;
	}

	*p1 = s;
	while (*s && !strchr(WHITESPACE SYMBOLS, *s)) {
		s++;
	}
	*p2 = s;
	return 'w';
}

int gettoken(char *s, char **p1) {
	static int c, nc;
	static char *np1, *np2;

	if (s) {
		nc = _gettoken(s, &np1, &np2);
		return 0;
	}
	c = nc;
	*p1 = np1;
	nc = _gettoken(np2, &np1, &np2);
	return c;
}

#define MAXARGS 128

void execute_jobs() {
	syscall_print_jobs();
}

void execute_fg(int jobId) {
	int envid = syscall_fg_job(jobId);
	if (envid == -1) {
		printf("fg: job (%d) do not exist\n", jobId);
	} else if (envid == -2) {
		printf("fg: (0x%08x) not running\n", envid);
	} else {
		wait(envid);
	}
}

void execute_kill(int jobId) {
	syscall_kill_job(jobId);
}

void create_history() {
	int fd;
	if ((fd = open(".mosh_history", O_CREAT)) < 0) {
		printf("create history file failed\n");
	}
	return;
}

void save_history_cmd(char * cmd) {
	int fd;
	if ((fd = open(".mosh_history", O_WRONLY)) < 0) {
		printf("open history file failed\n");
	}
	struct Stat st;
	stat(".mosh_history", &st);
	seek(fd, st.st_size);
	write(fd, cmd, strlen(cmd));
	write(fd, "\n", 1);
	close(fd);
	return;
}

void print_history() {
	int fd;
	if ((fd = open(".mosh_history", O_RDONLY)) < 0) {
		printf("open history file failed\n");
	}
	char ch;
	while (read(fd, &ch, 1)) {
		printf("%c", ch);
	}
	close(fd);
	return;
}

int parsecmd(char **argv, int *rightpipe, int mark) {
	int argc = 0;
	while (1) {
		char *t;
		int fd, r;
		int c = gettoken(0, &t);
		flag = 0;
		switch (c) {
		case 0:
			return mark ? argc : 0;
		case 'w':
			job_flag = 0; // 读到下一个单词说明有新指令 可以归0
			if (argc >= MAXARGS) {
				debugf("too many arguments\n");
				exit();
			}
			argv[argc++] = t;
			break;
		case '<':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: < not followed by word\n");
				exit();
			}
			// Open 't' for reading, dup it onto fd 0, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (1/3) */
			if ((fd = open(t, O_RDONLY)) < 0) {
				debugf("failed to open %s\n");
				exit();
			}
			if ((r = dup(fd, 0)) < 0) {
				debugf("failed to duplicate file to <stdin>\n");
				exit();
			}
			close(fd);
			break;
		case '>':
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: > not followed by word\n");
				exit();
			}
			// Open 't' for writing, create it if not exist and trunc it if exist, dup
			// it onto fd 1, and then close the original fd.
			// If the 'open' function encounters an error,
			// utilize 'debugf' to print relevant messages,
			// and subsequently terminate the process using 'exit'.
			/* Exercise 6.5: Your code here. (2/3) */
			if ((fd = open(t, O_TRUNC)) < 0) {
				debugf("failed to open %s\n", t);
				exit();
			}
			if ((r = dup(fd, 1)) < 0) {
				debugf("failed to duplicate file to <stdout>\n");
				exit();
			}
			close(fd);
			break;
		case '|':;
			/*
			 * First, allocate a pipe.
			 * Then fork, set '*rightpipe' to the returned child envid or zero.
			 * The child runs the right side of the pipe:
			 * - dup the read end of the pipe onto 0
			 * - close the read end of the pipe
			 * - close the write end of the pipe
			 * - and 'return parsecmd(argv, rightpipe)' again, to parse the rest of the
			 *   command line.
			 * The parent runs the left side of the pipe:
			 * - dup the write end of the pipe onto 1
			 * - close the write end of the pipe
			 * - close the read end of the pipe
			 * - and 'return argc', to execute the left of the pipeline.
			 */
			int p[2];
			/* Exercise 6.5: Your code here. (3/3) */
			if ((r = pipe(p)) < 0) {
				debugf("failed to create pipe\n");
				exit();
			}
			if ((*rightpipe = fork()) == 0) {
				dup(p[0], 0);
				close(p[0]);
				close(p[1]);
				return parsecmd(argv, rightpipe, 1);
			} else {
				dup(p[1], 1);
				close(p[1]);
				close(p[0]);
				return argc;
			}
			break;
		case ';':;
			int child = fork();
			if (child == 0) { // child shell
				return argc;
			} else { // parent shell
				wait(child); 
				return parsecmd(argv, rightpipe, 1);
			}
			break;
		case '&':;
			job_flag = 1;
			int child1 = fork();
			if (child1 == 0) { // child shell 后台指令
				return argc;
			} else { // parent shell
				return parsecmd(argv, rightpipe, 1);
			}		
			break;	
		case 'a':; // &&
			flag = 1;
			int child2 = fork();
			if (child2 == 0) { // child shell
				return mark ? argc : 0;
			} else { // parent shell
				syscall_recv_return_value();
				if (env->return_value == 0) {
					return parsecmd(argv, rightpipe, 1);
				} else {
					return parsecmd(argv, rightpipe, 0);
				}
			}
			break;
		case 'o':; // ||
			flag = 1;
			int child3 = fork();
			if (child3 == 0) {
				return mark ? argc : 0;
			} else {
				syscall_recv_return_value();
				if (env->return_value != 0) {
					return parsecmd(argv, rightpipe, 1);
				} else {
					return parsecmd(argv, rightpipe, 0);
				}
			}
			break;
		case 'z':; // 实现追加重定向
			if (gettoken(0, &t) != 'w') {
				debugf("syntax error: > not followed by word\n");
				exit();
			}
			if ((fd = open(t, O_RDONLY | O_WRONLY | O_CREAT)) < 0) {
				debugf("failed to open %s\n", t);
				exit();
			}
			struct Stat st;
			if (fstat(fd, &st) < 0) {
				debugf("failed to fstat %s\n", t);
				exit();
			}
			seek(fd, st.st_size);
			if ((r = dup(fd, 1)) < 0) {
				debugf("failed to duplicate file to <stdout>\n");
				exit();
			}
			close(fd);
			break;	
		case 'f':; // 反引号 
			runcmd(t);
			break;
		}
	}

	return argc;
}

char cmd[1024]; // 存储运行指令

void runcmd(char *s) {
	/*Shell Challenge: #*/
	char *p = s;
	while (*p && *p != '#') {
		p++;
	}
	*p = 0;
	gettoken(s, 0);

	char *argv[MAXARGS];
	int rightpipe = 0; // rightpipe is the envid of the right side of a pipe | 
	int argc = parsecmd(argv, &rightpipe, 1);
	if (argc == 0) {
		if (argv[0]) { // 后边还有指令
			syscall_send_return_value(env->env_parent_id, 0, 0, 0);
		}
		return;
	}
	argv[argc] = 0;

	/*Shell Challenge : built-in command*/
	if (strcmp(argv[0], "jobs") == 0) {
		execute_jobs();
		exit();
	} else if (strcmp(argv[0], "fg") == 0) {
		int jobId = parseJobId(argv[1]);
		execute_fg(jobId);
		exit();
	} else if (strcmp(argv[0], "kill") == 0) {
		int jobId = parseJobId(argv[1]);
		execute_kill(jobId);
		exit();
	} else if (strcmp(argv[0], "history") == 0) {
		print_history();
		exit();
	}

	/*external command*/
	int child = spawn(argv[0], argv); // spawn a new process to run the command
	// if succeeds, child is the envid of the new process.
	// if fails, child is the error code. 
	if (child >= 0) {
		if (flag == 1) {
			syscall_recv_return_value();
			syscall_send_return_value(env->env_parent_id, env->return_value, 0, 0);
		} else if (job_flag == 1) { // 需要创建后台任务 
			syscall_create_job(child, cmd);
			syscall_recv_return_value();
		} else {
			syscall_recv_return_value();
		}
	} else {
		debugf("spawn %s: %d\n", argv[0], child);
	}
	if (rightpipe) {
		wait(rightpipe);
	}
	close_all(); // close all file descriptors
	exit();
}

int parseJobId(char *s) {
	int jobId = 0;
	while (*s) {
		jobId = jobId * 10 + (*s - '0');
		s++; 
	}
	return jobId;
}

void readline(char *buf, u_int n) {
	int r;
	for (int i = 0; i < n; i++) {
		if ((r = read(0, buf + i, 1)) != 1) {
			if (r < 0) {
				debugf("read error: %d\n", r);
			}
			exit();
		}
		if (buf[i] == '\b' || buf[i] == 0x7f) {
			if (i > 0) {
				i -= 2;
			} else {
				i = -1;
			}
			if (buf[i] != '\b') {
				printf("\b");
			}
		}
		if (buf[i] == '\r' || buf[i] == '\n') {
			buf[i] = 0;
			return;
		}
	}
	debugf("line too long\n");
	while ((r = read(0, buf, 1)) == 1 && buf[0] != '\r' && buf[0] != '\n') {
		;
	}
	buf[0] = 0;
}

char buf[1024];

void usage(void) {
	printf("usage: sh [-ix] [script-file]\n");
	exit();
}


int main(int argc, char **argv) {
	int r;
	int interactive = iscons(0);
	int echocmds = 0;
	printf("\n:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	printf("::                                                         ::\n");
	printf("::                     MOS Shell 2024                      ::\n");
	printf("::                                                         ::\n");
	printf(":::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::\n");
	ARGBEGIN {
	case 'i':
		interactive = 1;
		break;
	case 'x':
		echocmds = 1;
		break;
	default:
		usage();
	}
	ARGEND

	if (argc > 1) {
		usage();
	}
	if (argc == 1) {
		close(0);
		if ((r = open(argv[0], O_RDONLY)) < 0) {
			user_panic("open %s: %d", argv[0], r);
		}
		user_assert(r == 0);
	}
	create_history();
	for (;;) {
		if (interactive) {
			printf("\n$ ");
		}
		readline(buf, sizeof buf);

		if (buf[0] == '#') {
			continue;
		}
		if (echocmds) {
			printf("# %s\n", buf);
		}
		if ((r = fork()) < 0) {
			user_panic("fork: %d", r);
		}
		if (r == 0) { // 子进程shell执行指令
			save_history_cmd(buf);
			strcpy(cmd, buf);
			runcmd(buf);
			exit();
		} else { // 父进程shell等待 
			wait(r);
		}
	}
	return 0;
}
