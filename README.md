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

