#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include"server.h"

int main(int argc,char* argv[]) {
	if (argc < 3) {
		printf("./a.out port respath\n");
		exit(0);
	}
	//����������������epoll
	chdir(argv[2]);	//�������л������̵ĸ�Ŀ¼��
	unsigned short port = atoi(argv[1]);
	epollRun(port);
	return 0;
}