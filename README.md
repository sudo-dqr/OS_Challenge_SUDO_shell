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



## 十.实现前后台任务管理





