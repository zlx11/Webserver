#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include"server.h"

int main(int argc,char* argv[]) {
	if (argc < 3) {
		printf("./a.out port respath\n");
		exit(0);
	}
	//启动服务器，基于epoll
	chdir(argv[2]);	//将进程切换到进程的根目录中
	unsigned short port = atoi(argv[1]);
	epollRun(port);
	return 0;
}