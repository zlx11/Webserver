#include"server.h"
#include<arpa/inet.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<string.h>
#include<strings.h>
#include<errno.h>
#include<stdio.h>
#include<dirent.h>
#include<sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
int initListenFd(unsigned short port)
{
	//2.设置监听的套接字
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1) {
		perror("socket");
		return -1;
	}
	//2.设置端口复用
	int opt = 1;
	int ret = setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if (ret == -1) {
		perror("setsockopt");
		return -1;
	}
	//3.绑定
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	ret = bind(lfd, (struct sockaddr*)&addr, sizeof(addr));
	if (ret == -1) {
		perror("bind");
		return -1;
	}
	//4.设置监听
	ret = listen(lfd, 128);
	if (ret == -1) {
		perror("listen");
		return -1;
	}
	//5.将得到的可用的套接字返回给调用者
	return lfd;
}

int epollRun(unsigned short port)
{
	//1.创建epoll模型
	int epfd = epoll_create(10);
	//2.初始化epoll模型
	int lfd = initListenFd(port);
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = lfd;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, lfd, &ev);
	if (ret == -1) {
		perror("epoll_ctl");
		return -1;
	}
	//检测
	struct epoll_event evs[1024];
	int size = sizeof(evs) / sizeof(evs[0]);
	int flag = 0;
	while (1) {
		if (flag) {
			break;
		}
		int num = epoll_wait(epfd, evs, size, -1);
		for (int i = 0; i < num; i++) {
			int curfd = evs[i].data.fd;
			if (curfd == lfd) {
				//建立新连接
				int ret = acceptConn(lfd, epfd);
				if (ret == -1) {
					flag = 1;
					break;
				}
			}
			else {
				//通信
				recvHttpRequest(curfd, epfd);
			}
		}
	}
	return 0;
}

int acceptConn(int lfd, int epfd)
{
	//1.建立新连接
	int cfd = accept(lfd, NULL, NULL);
	if (cfd == -1) {
		perror("accept");
		return -1;
	}

	//2.设置通信文件描述符为非阻塞
	int flag = fcntl(cfd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(cfd, F_SETFL, flag);

	//3.通信的文件描述符添加到epoll模型中
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;	//边沿模式
	ev.data.fd = cfd;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, cfd, &ev);
	if (ret == -1) {
		perror("epoll_ctl");
		exit(0);
	}
	return 0;
}

int recvHttpRequest(int cfd, int epfd)
{
	char tmp[1024];
	char buf[4096];
	ssize_t len, total = 0;
	//total 和 len 应该在 buf 大小限制下进行适当检查
	while ((len = recv(cfd, tmp, sizeof(tmp), 0)) > 0) {
		if (total + len < sizeof(buf)) {
			memcpy(buf + total, tmp, (size_t)len);
			total += len;
		}
		else {
			printf("Request too large.\n");
			break;
		}
	}
	//读完了
	if (len == -1 && errno == EAGAIN) {
		char* pt = strstr(buf, "\r\n");
		long reqlen = pt - buf;
		buf[reqlen] = '\0';
		//解析请求行
		parseRequestLine(buf, cfd);
	}
	else if (len == 0) {
		printf("The client has been disconnected...\n");
		disConnect(cfd, epfd);
	}
	else {
		perror("recv");
		return -1;
	}
	return 0;
}

int disConnect(int cfd, int epfd)
{
	int ret = epoll_ctl(epfd, EPOLL_CTL_DEL, cfd, NULL);
	if (ret == -1) {
		perror("epoll_ctl");
		close(cfd);
		return -1;
	}
	close(cfd);
	return 0;
}

int parseRequestLine(const char* reqLine, int cfd)
{
	char method[6];
	char path[1024];
	if (sscanf(reqLine, "%5s %1023s", method, path) != 2) {
		printf("Invalid request line format\n");
		return -1;
	}
	sscanf(reqLine, "%[^ ] %[^ ]", method, path);
	if (strcasecmp(method, "GET") == 0) {
		char* file = NULL;
		//如果有中文需还原
		decodeMsg(path, path);
		if (strcmp(path, "/") == 0) {
			file = "./";
		}
		else {
			file = path + 1;
		}
		printf("Parsed path: %s\n", path);
		printf("File name or directory name:%s\n", file);

		struct stat st;
		int ret = stat(file, &st);
		if (ret == -1) {
			perror("File not found or invalid path");
			sendHeadMsg(cfd, 404, "Not Found", get_mime_type(".html"), -1);
			sendFile(cfd, "404.html");
		}
		if (S_ISDIR(st.st_mode)) {
			sendHeadMsg(cfd, 200, "OK", get_mime_type(".html"), -1);
			sendDir(cfd, file);
		}
		else {
			sendHeadMsg(cfd, 200, "OK", get_mime_type(file), (int)st.st_size);
			sendFile(cfd, file);
		}
	}
	else {
		printf("用户提交的不是git请求，忽略...\n");
		return -1;
	}
	return 0;
}

int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length) {
	char buf[1024];
	sprintf(buf, "HTTP/1.1 %d %s\r\n", status, descr);
	//sprintf(buf + strlen(buf), "Content-Type: %s\r\n", type);  // 确保这里是text/html
	sprintf(buf + strlen(buf), "Content-Type: %s; charset=utf-8\r\n", type);

	sprintf(buf + strlen(buf), "Content-Length: %d\r\n", length);
	sprintf(buf + strlen(buf), "Content-Disposition: inline\r\n");  // 显示内容
	sprintf(buf + strlen(buf), "\r\n");
	send(cfd, buf, strlen(buf), 0);
	return 0;
}



int sendFile(int cfd, const char* fileName)
{
	int fd = open(fileName, O_RDONLY);
	while (1) {
		char buf[1024] = { 0 };
		ssize_t len = read(fd, buf, sizeof(buf));
		if (len > 0) {
			send(cfd, buf, (size_t)len, 0);
			usleep(50);
		}
		else if (len == 0) {
			break;
		}
		else {
			perror("读文件失败...\n");
			return -1;
		}
	}
	return 0;
}



int sendDir(int cfd, const char* dirName) {
	char buf[4096];
	struct dirent** namelist;
	sprintf(buf, "<html><head><title>%s</title></head><body><table>", dirName);
	int num = scandir(dirName, &namelist, NULL, alphasort);
	for (int i = 0; i < num; ++i) {
		char* name = namelist[i]->d_name;
		char subpath[1024];
		sprintf(subpath, "%s/%s", dirName, name);
		struct stat st;
		stat(subpath, &st);
		if (S_ISDIR(st.st_mode)) {
			sprintf(buf + strlen(buf), "<tr><td><a href=\"%s/\">%s</a></td><td>%d</td></tr>", name, name, (long)st.st_size);
		}
		else {
			sprintf(buf + strlen(buf), "<tr><td><a href=\"%s\">%s</a></td><td>%d</td></tr>", name, name, (long)st.st_size);
		}
		send(cfd, buf, strlen(buf), 0);
		memset(buf, 0, sizeof(buf));  // 清空缓冲区，避免数据混乱
		free(namelist[i]);
	}
	sprintf(buf, "</table></body></html>");
	send(cfd, buf, strlen(buf), 0);
	free(namelist);
	return 0;
}

const char* get_mime_type(const char* filename) {
	// 获取文件名的扩展名
	const char* dot = strrchr(filename, '.');

	if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0) {
		return "text/html";
	}
	else if (strcmp(dot, ".txt") == 0) {
		return "text/plain";
	}
	else if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) {
		return "image/jpeg";
	}
	else if (strcmp(dot, ".png") == 0) {
		return "image/png";
	}
	else if (strcmp(dot, ".gif") == 0) {
		return "image/gif";
	}
	else if (strcmp(dot, ".pdf") == 0) {
		return "application/pdf";
	}
	else if (strcmp(dot, ".zip") == 0) {
		return "application/zip";
	}
	else if (strcmp(dot, ".json") == 0) {
		return "application/json";
	}
	else if (strcmp(dot, ".css") == 0) {
		return "text/css";
	}
	else if (strcmp(dot, ".js") == 0) {
		return "application/javascript";
	}
	else {
		return "application/octet-stream";  // 未知类型，返回默认
	}
}

int hexit(char c) {
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'a' && c <= 'f')
		return c - 'a' + 10;
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return 0;
}

void decodeMsg(char* to, char* from) {
	for (; *from != '\0'; ++to,++from) {
		if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {
			*to = hexit(from[1]) * 16 + hexit(from[2]);
			from += 2;
		}
		else {
			*to = *from;
		}
	}
	*to = '\0';
}

