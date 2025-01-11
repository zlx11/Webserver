#ifndef SERVER_H
	
#define SERVER_H
int initListenFd(unsigned short port);
int epollRun(unsigned short port);
int acceptConn(int lfd, int epfd);
int recvHttpRequest(int cfd,int epfd);
int disConnect(int cfd,int epfd);
int parseRequestLine(const char* reqLine,int cfd);
int sendHeadMsg(int cfd, int status, const char* descr, const char* type, int length);
int sendFile(int cfd,const char* fileName);
int sendDir(int cfd, const char* dirName);
const char* get_mime_type(const char* filename);
int hexit(char c);
void decodeMsg(char* to, char* from);
#endif // !SERVER_H
