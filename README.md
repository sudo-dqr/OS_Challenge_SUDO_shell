# Shell挑战性任务设计文档:MOS_SUDO_SHELL

## 一.实现不带```.b```的后缀指令

​	实现不带```.b```的后缀指令同时兼容带有```.b```后缀的指令，这一点可以通过**首先尝试打开不带后缀的指令名，例如```ls```，失败后再尝试打开带后缀的指令名```ls.b```实现。**修改```spawn.c/spawn```函数中的打开文件逻辑即可。

```c
int spawn(char *prog, char **argv) {
	// Step 1: Open the file 'prog' (the path of the program).
	// Return the error if 'open' fails.
	int fd;
	if ((fd = open(prog, O_RDONLY)) < 0) {
		/*Shell Challenge: *.b*/
		char *ext = ".b";
		char prog_b[1024];
		strcpy(prog_b, prog);
		// strcat
		for (int i = strlen(prog_b), j = 0; j < strlen(ext); i++, j++) {
			prog_b[i] = ext[j];
		}
		if ((fd = open(prog_b, O_RDONLY)) < 0) {
			return fd;
		}
	}
    //...
}
```

## 二.实现指令条件执行

​	需要实现```Linux Shell```中的```&&```与```||```，需要满足短路原则。

* 对于```command1 && command2```，```commmand2```被执行当且仅当```command1```返回0
* 对于```command1 | command2```，```command2```被执行当且仅当```command1```返回1
* **并且```&&```和```||```的优先级相同，从左向右执行**

​	课程组已经给出了测试程序```true.c```和```false.c```，其中```true.c```返回0,```false.c```返回1,可以增加适当的输出信息用于测试。

​	首先应当想到在解析命令时新增两种```token```：```&&```和```||```，分别用首字母```a```和```o```表示。

```c
int _gettoken(char *s, char **p1, char **p2) {
    //...
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
		}
		*s++ = 0;
		*p2 = s;
		return t;
	}
    //...
}
```

​	当我们在```parsecmd```中解析到```&&```或```||```时，我们的思路是先```fork```一下，首先运行左侧的指令，是否运行右边的指令由左边指令的返回值确定，返回值需要通过进程间通信实现。大致梳理一下这个过程：

* parent
  * child
    * spawn : grandson

​	我们可以看出这个**通信过程是一个由底向上的过程**，我们知道指令的具体执行是由子shell进行spawn后的“孙子”进程完成的，那么就要由“晚辈的”一层层向“长辈”靠拢，这其间的通信我们使用```ipc```实现。

​	```grandson```的返回值在```libos.c/libmain```中接收，并返回给```child```，需要注意的是除了这里```debugf.c/user_panic```中也要进行通信，我选择返回-1表示指令错误执行。

```c
// libos.c
void libmain(int argc, char **argv) {
	// set env to point at our env structure in envs[].
	env = &envs[ENVX(syscall_getenvid())];

	// call user main routine
	u_int r = main(argc, argv); 
	syscall_ipc_try_send(env->env_parent_id, r, 0, 0);
	// exit gracefully
	exit();
}
// debugf.c
void _user_panic(const char *file, int line, const char *fmt, ...) {
	debugf("panic at %s:%d: ", file, line);
	va_list ap;
	va_start(ap, fmt);
	vdebugf(fmt, ap);
	va_end(ap);
	syscall_ipc_try_send(env->env_parent_id, -1, 0, 0);
	debugf("\n");
	exit();
}
```

* ```child```进程使用```ipc_recv```接收返回值，**这里需要注意的是，使用syscall_ipc_recv后，并不需要再进行wait，因为接收到返回值已经说明子进程执行完毕。**
* 另外需要注意的是，当我们遇到```&& ||```时```child```需要向```parent```进行通信，其他时候不需要，我们可以设置一个标志```flag```进行区分。

```c
void runcmd(char *s) {
	// ...
	int child = spawn(argv[0], argv); // spawn a new process to run the command
	// if succeeds, child is the envid of the new process.
	// if fails, child is the error code. 
	close_all(); // close all file descriptors
	if (child >= 0) {
		syscall_ipc_recv(0);
		if (flag == 1) {
			syscall_ipc_try_send(env->env_parent_id, env->env_ipc_value, 0, 1);
		}
	} else {
		debugf("spawn %s: %d\n", argv[0], child);
	}
	//...
}
```

​	考虑到一种比较复杂的情况是```cmd1 || cmd2 || cmd3 ```等可能省略中间部分指令的情况，我选择给```parsecmd```增加一个参数```mark```来表示这条指令是否需要执行，经过梳理运行逻辑，一种可行的解决办法是：当我们遇到**短路原则**时，下一条指令是一定不需要执行的，我们将```mark```标记为0，当```return```时，若```mark```标记为0,则```return 0```，否则```return argc```。在```runcmd```原有的逻辑中，```return 0 ```说明后面没有命令，会退出命令运行，**但在此时，我们不能够退出，我们的目标是跳过一条指令，还需要判断后边的指令是否需要执行**。**我们可以对return 0的情况进行分析**

1. 命令末尾：```return 0 && argv[0] = ""```
2. 若是省略命令：```return 0 && argv[0] != ""```

​	**因此我们可以利用```argv[0]```是否为空串来对这两种情况进行区分**，当是第一种情况时，直接退出；当是第二种情况时，我们需要与代表着```cmd3```的父进程进行通信，**这时我们将前两个已经短路的指令看作一个整体，返回0**。

* ```parsecmd```中注意细节，每一个```return```都要对```mark```进行判断。

```c
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
		//...
		case 'a':; // &&
			flag = 1;
			int child2 = fork();
			if (child2 == 0) { // child shell
				return mark ? argc : 0;
			} else { // parent shell
				syscall_ipc_recv(0);
				if (env->env_ipc_value == 0) {
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
				syscall_ipc_recv(0);
				if (env->env_ipc_value != 0) {
					return parsecmd(argv, rightpipe, 1);
				} else {
					return parsecmd(argv, rightpipe, 0);
				}
			}
			break;
		}
	}

	return argc;
}

void runcmd(char *s) {
	//...
	if (argc == 0) {
		if (argv[0]) { // 后边还有指令
			syscall_ipc_try_send(env->env_parent_id, 0, 0, 0);
		}
		return;
	}
	argv[argc] = 0;
	//...
}
```

## 三.实现更多指令

### 3.1 ```touch```

* ```touch <file>```：不会出现创建多个文件的情况

​	新建```touch.c```并在```include.mk```中加入```touch.b```，**检查文件是否存在的方式为先尝试打开，若不能成功打开则进行创建。**

```c
// touch.c
#include<lib.h>

void touch(char *path) {
    int fd = open(path, O_RDONLY);
    // 首先检查文件是否存在
    if (fd > 0) {
        close(fd);
        return;
    } else {
        // 文件不存在，创建文件
        fd = open(path, O_CREAT);
        if (fd < 0) {
            printf("touch: cannot touch '%s': No such file or directory\n", path);
            return;
        }
        close(fd);
    
    }
}

int main(int argc, char **argv) {
    int i;
    if (argc < 2) {
        printf("touch: missing file operand\n");
        return 0;
    }
    touch(argv[1]);
}
```

### 3.2 ```mkdir```

* ```mkdir <dir>```
* ```mkdir -p <dir>```

​	```mkdir```的实现方式与```touch```类似，在```open```时传入```O_MKDIR```，需要注意的是要在文件系统服务函数```serv.c```中打开函数时相应加入对于```O_MKDIR```的判断。

```c
 void serve_open(u_int envid, struct Fsreq_open *rq) {
	//...
	if ((rq->req_omode & O_CREAT) && (r = file_create(rq->req_omode, rq->req_path, &f)) < 0 &&
	    r != -E_FILE_EXISTS) {
		ipc_send(envid, r, 0, 0);
		return;
	} else if ((rq->req_omode & O_MKDIR) && (r = file_create(rq->req_omode, rq->req_path, &f)) < 0 &&
	    r != -E_FILE_EXISTS) {
		ipc_send(envid, r, 0, 0);
		return;
	}
     //...
}
```

​	同时应当修改```file_create```函数，增加传入参数```rq->req_mode```，区分创建文件还是目录，为结构体的```type```字段赋值。

```c
int file_create(u_int req_mode, char *path, struct File **file) {
    //...
	/*Shell Challenge*/
	if (req_mode == O_MKDIR) {
		f->f_type = FTYPE_DIR;
	} else {
		f->f_type = FTYPE_REG;
	}
	//...
}
```

​	具体的``mkdir``逻辑

```c
#include<lib.h>

void mkdir(char *path, int flag) {
    int fd = open(path, O_RDONLY); // 首先检查文件是否存在
    if (fd > 0) {
        close(fd);
        if (!flag) { 
            printf("mkdir: cannot create directory '%s': File exists\n", path);
        }
        return;
    } else { // 文件不存在，创建文件
        fd = open(path, O_MKDIR);
        if (fd < 0) {
            if (!flag) {
                printf("mkdir: cannot create directory '%s': No such file or directory\n", path);
                return;
            } else { // 递归创建目录,例如 "/nonexist/dqr/123"
                char *p = path;
                if (*p == '/') { // 跳过开头的'/'
                    p++;
                }
                while (1) {
                    if (*p == '/') {
                        *p = '\0';
                        mkdir(path, 1);
                        *p = '/';
                    } else if (*p == '\0') {
                        mkdir(path, 1);
                        break;
                    }
                    p++;
                }
            }
        }
        close(fd);
    }
}

int main(int argc, char **argvs) {
    if (argc < 2) {
        printf("mkdir: missing operand\n");
        return 0;
    }
    // 需要考虑参数p忽略错误
    if (strcmp(argvs[1], "-p") == 0) {
        mkdir(argvs[2], 1);
    } else {
        mkdir(argvs[1], 0);
    }
    return 0;
}
```

### 3.3 ```rm```

* ```rm <file>```
* ```rm <dir>```
* ```rm -r <dir>|<file>```
* ```rm -rf <dir>|<file>```

​	对于```rm```指令处理的关键在于对于参数的处理，我们知道删除目录时需要```-r/-rf```参数，所以需要特殊判断无参数删除目录文件的情况。我采用的方法是在文件系统删除请求结构体中加入一个模式字段，用来表示参数情况(```无参数为0，-r为1，-rf为2```)。

```c
struct Fsreq_remove {
	char req_path[MAXPATHLEN];
	u_int remove_type;
};
```

​	对于这个参数的传递，我选择了一种不太“优雅“的方式，由于不能修改```remove```函数的传参(remove函数在test中有引用，评测时替换为标准文件，若修改会导致编译不过)，**我选择将模式数字拼接在路径字符串的\0之后，这样我们既可以读取模式数字，又可以保证读取路径的正确性。**

​	在```remove```函数中将这个数字取出，沿着调用链进行传递。

```c
int remove(const char *path) {
	// Call fsipc_remove.
	// 最后一位为模式
	int len = strlen(path);
	int type = path[++len] - '0';
	/* Exercise 5.13: Your code here. */
	return fsipc_remove(path, type);
}
```

​	在```fsipc_remove```中构建请求结构体时对新增字段赋值，达到通过结构体传参的目的。

```c
int fsipc_remove(const char *path, u_int type) {
	//...
	req->remove_type = type;
	//...
}
```

​	在文件系统服务函数中，为```file_remove```新增模式参数

```c
void serve_remove(u_int envid, struct Fsreq_remove *rq) {
	r = file_remove(rq->req_path, rq->remove_type);
	ipc_send(envid, r, 0, 0);
}
```

​	修改具体的文件删除逻辑，当要删除的文件为目录但传递的参数为N(0)时，返回一种新的错误码(```E_IS_DIR=14,新增定义在error.h中```)

```c
int file_remove(char *path, u_int type) {
	int r;
	struct File *f;

	// Step 1: find the file on the disk.
	if ((r = walk_path(path, 0, &f, 0)) < 0) {
		return r;
	}

	/*Shell Challenge*/
	if (f->f_type == FTYPE_DIR && type == 0) {
		return -E_IS_DIR;
	}
    //...
}
```

​	具体的```remove```逻辑：这里返回小于0值说明一定是N模式下删除了目录。

```c
#include <lib.h>

#define N 0
#define R 1
#define RF 2

void rm(char *path, int flag) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        if (flag != RF) {
            printf("rm: cannot remove '%s': No such file or directory\n", path);
        }
    } else {
        close(fd);
        /*为了避免传参问题 将模式拼接在路径末尾\0之后*/
        char path1[MAXPATHLEN+1];
        strcpy(path1, path);
        int len = strlen(path1);
        path1[++len] = flag + '0';
        /********************/
        fd = remove(path1);
        if (fd < 0) {
            printf("rm: cannot remove '%s': Is a directory\n", path);
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("rm: missing operand\n");
    }
    if (strcmp(argv[1], "-r") == 0) {
        rm(argv[2], R);
    } else if (strcmp(argv[1], "-rf") == 0) {
        rm(argv[2], RF);
    } else {
        rm(argv[1], N);
    }
    return 0;
}

```

## 四.实现反引号

​	使用反引号实现指令替换，**只需要考虑```echo```进行的输出**，需要将反引号内指令执行的所有标准输出替换为```echo```的参数。



## 五.实现注释功能

​	使用```#```实现注释功能也就是对```#```后面的内容进行忽略，可以在进行执行指令时对字符串```#```后面的部分进行截断。

```c
//sh.c
void runcmd(char *s) {
	/*Shell Challenge: #*/
	char *p = s;
	while (*p && *p != '#') {
		p++;
	}
	*p = 0;
	gettoken(s, 0);
    //...
}
```

## 六.实现历史指令



## 七.实现一行多指令

​	实现使用```;```将多条指令隔开从而从左至右依顺序执行每条指令的功能，通过修改```parsecmd()```函数使其增添对```;```的处理能力。在```parsecmd```的字符串解析过程中，若读到一个```;```，则进行一次```fork```，产生一个子```shell```，让子```shell```执行左边的命令，父```shell```等待子```shell```执行完成后执行右边的命令。

```c
		case ';':;
			int child = fork();
			if (child == 0) { // child shell
				return argc;
			} else { // parent shell
				wait(child); 
				return parsecmd(argv, rightpipe);
			}
			break;
```

## 八.实现追加重定向

* 实现```>>```的追加重定向的功能

​	首先增加一种```token```为```>>```，在```_gettoken```中实现，标记为```z```。

```c
int _gettoken(char *s, char **p1, char **p2) {
	//...
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
	//...
}
```

​	在```parsecmd```中进行修改，为文件设置好偏移量后进行```dup```

* ```stat```获取文件信息
* ```seek```设置文件偏移量

```c
int parsecmd(char **argv, int *rightpipe, int mark) {
	int argc = 0;
	while (1) {
		//...
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
		}
	}

	return argc;
}
```

## 九.实现引号支持

​	```shell```在解析时需要将双引号内的内容看作是单个字符串，在解析字符串时加了一种```token```为字符串，修改```_gettoken```中的逻辑。

```c
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
```

## 十.实现前后台任务管理

### 10.1 实现后台任务并行

​	此功能的实现与实现一行多指令类似，后台运行任务即在```fork```之后父进程不会等待子进程运行完成。

```c
		case '&':;
			int child = fork();
			if (child == 0) { // child shell
				return argc;
			} else { // parent shell
				return parsecmd(argv, rightpipe);
			}		
			break;	
```

### 10.2 实现```jobs```指令

> 关于后台任务的三个指令我们需要实现为内置指令，shell中分为内置指令和外部指令，外部指令需要fork一个子进程进行执行，而内置指令不会，效率更高。
>
> * 基本的实现思路为我们在runcmd时检查指令，如果为内置指令不再进行```spawn```而是直接调用```sh.c```中写好的函数执行。
> * 为了实现进程间的共享，我选择在内核态对后台进程进行管理，通过系统调用```syscall_*```进行更新



### 10.3 实现```fg```指令

* 首先构建一个简单的辅助函数，用来将```argv[1]```转换为```<jobid>```

  ```c
  int parseJobId(char *s) {
  	int jobId = 0;
  	while (*s) {
  		jobId = jobId * 10 + (*s - '0');
  		s++; 
  	}
  	return jobId;
  }
  ```

  

### 10.4 ```kill```指令





