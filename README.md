HTTP Web Server (基于 epoll 的高效服务器)
该项目是一个简易的 HTTP Web 服务器，基于 epoll 实现高效的事件驱动模式，支持文件和目录的访问。它通过非阻塞 IO 与 epoll 多路复用机制进行通信，并能够处理 HTTP GET 请求。

功能概述
支持基本的 HTTP 请求解析，特别是对 GET 请求的处理。
能够处理文件请求，并返回文件内容（如 .html, .jpg, .png 等）。
支持目录浏览功能，返回目录列表和文件信息。
基于 epoll 事件驱动模型，支持高效的 I/O 操作。
项目结构
server.c: 主要的服务器逻辑，包含套接字的创建、监听、接受连接、请求处理等。
server.h: 头文件，包含所有函数声明及必要的宏定义。
Makefile: 编译和构建项目的文件。
使用说明
编译和运行
编译:

在项目目录下，使用 Makefile 来编译项目：

bash
make
运行:

使用命令行参数来指定服务器监听的端口和根目录路径：

bash
./a.out <port> <root_path>
<port>：监听的端口号。
<root_path>：服务器的根目录（即文件存储位置）。
例如，如果希望服务器监听端口 8080 并将文件根目录设置为当前路径，可以运行：

bash
./a.out 8080 .
HTTP 请求
当客户端访问服务器时，服务器将根据请求的 URL 返回相应的文件或目录。

例如，访问 http://localhost:8080/index.html 将返回服务器根目录下的 index.html 文件。
如果请求的是一个目录，服务器将返回该目录中的文件列表。
支持 HTTP GET 请求，若文件或目录不存在，将返回 404 错误页面。

支持的 MIME 类型
服务器根据文件扩展名自动返回合适的 MIME 类型。常见的类型包括：

text/html
text/plain
image/jpeg
image/png
application/pdf
application/json
application/zip
text/css
application/javascript
文件和目录响应
文件响应：当请求的是一个文件时，服务器会将文件内容读取并通过 HTTP 响应返回。
目录响应：当请求的是一个目录时，服务器会列出该目录中的文件和子目录，并允许点击进入子目录。
代码实现
主要功能函数
initListenFd: 初始化监听套接字，设置端口复用并开始监听。
epollRun: 创建和运行 epoll 事件循环，处理 I/O 事件。
acceptConn: 处理新连接，设置为非阻塞模式并添加到 epoll 事件中。
recvHttpRequest: 接收并解析 HTTP 请求。
parseRequestLine: 解析 HTTP 请求行（例如 GET /index.html HTTP/1.1）。
sendHeadMsg: 发送 HTTP 响应头。
sendFile: 发送文件内容。
sendDir: 发送目录内容，列出目录下的文件和子目录。
get_mime_type: 根据文件扩展名获取对应的 MIME 类型。
decodeMsg: 解码 URL 编码的字符串。
网络通信
使用 socket() 创建套接字，bind() 绑定端口，listen() 开始监听。
采用 epoll 来管理 I/O 事件，确保高效的非阻塞 I/O 操作。
贡献
欢迎提交 pull requests 或者创建 issues 提供改进意见和 bug 修复。
