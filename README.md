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

​	课程组已经给出了测试程序```true.c```和```false.c```，其中```true.c```返回0,```false.c```返回1,可以增加适当的输出信息用于测试。

​	主要思路为在```parsecmd```中进行功能扩展。



## 三.实现更多指令

> 以下三条指令涉及到与文件系统进程进行通信，添加过程较为类似

### 3.1 ```touch```

* ```touch <file>```

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
    for (i = 1; i < argc; i++) {
        touch(argv[i]);
    }
}
```

### 3.2 ```mkdir```

* ```mkdir <dir>```
* ```mkdir -p <dir>```

​	```mkdir```的实现方式与```touch```类似，在```open```时传入```O_MKDIR```，需要注意的是要在文件系统服务函数```serv.c```中打开函数时相应加入对于```O_MKDIR```的判断。

### 

### 3.3 ```rm```



## 四.实现反引号

​	使用反引号实现指令替换，只需要考虑```echo```进行的输出，需要将反引号内指令执行的所有标准输出替换为```echo```的参数

## 五.实现注释功能



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



### 10.3 实现```fg```指令



### 10.4 ```kill```指令





