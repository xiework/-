#include "utility.h"

int my_connect()
{
	char buf[BUF_SIZE];
	sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERVER_PORT);
	inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr.s_addr);
	int clie_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (clie_fd == -1) {
		perror("socket error");
		exit(1);
	}
	int ret = connect(clie_fd, (sockaddr*)&serv_addr, sizeof(serv_addr));
	if (ret == -1) {
		perror("connect error");
		exit(1);
	}
	ret = recv(clie_fd, buf, sizeof(buf), 0);
	printf("%s\n", buf);
	return clie_fd;
}
int main()
{
	//1.建立客户端socket cfd,并与服务端连接;
	int clie_fd = my_connect();
	//2.创建无名管道
	int pipe_[2];
	int ret = pipe(pipe_);
	if (ret == -1) {
		perror("pipe error");
		exit(1);
	}
	//3.创建监听树
	int epfd = epoll_create(EPOLL_SIZE);
	//4.将clie_fd 和pipe_[0]挂到监听树上
	epoll_add(epfd, clie_fd);
	epoll_add(epfd, pipe_[0]);
	//客户端是否还正常工作
	bool isClientwork = true;
	//读写缓冲区
	char buf[BUF_SIZE] = {0};
	//4.创建多进程
	int pid = fork();
	if (pid == 0) {//子进程负责写数据到管道
		close(pipe_[0]);//子进程关闭管道读端
		int ret;
		while(isClientwork) {//判断子进程是否正常工作
			bzero(buf,sizeof(buf));
			fgets(buf, sizeof(buf), stdin);//读取客户端的输入
			//printf("客户端:子进程:%s\n", buf);
			if (strncasecmp(buf, EXIT, strlen(EXIT)) == 0) {//判断用户是否要退出聊天室
				isClientwork = false;
				continue;
			}
			ret = write(pipe_[1], buf, strlen(buf));//向管道写数据
			if (ret < 0) {
				perror("write error");
				exit(1);
			}
			
		}

	} else {//父进程负责读管道和套接字
		//关闭管道写端
		close(pipe_[1]);
		epoll_event events[2];
		int ret;
		while (isClientwork) {
			int event_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
			//有事件返回
			for (int i = 0; i < event_count; ++i) {
				bzero(buf, sizeof(buf));
				epoll_event ev = events[i];
				if (ev.data.fd == clie_fd) {//服务端有消息，将消息显示在终端
					ret = recv(ev.data.fd, buf, sizeof(buf), 0);
					if (ret == 0) {
						printf("服务端关闭\n");
						close(pipe_[0]);
						close(epfd);
						close(clie_fd);
						isClientwork = false;
					} else if (ret < 0) {
						printf("recv error");
						exit(1);
					} else {
						printf("%s", buf);
					}
				} else {//子进程有消息
					ret = read(ev.data.fd, buf, sizeof(buf));
					//printf("客户端父进程：%s\n",buf);
					if (ret < 0) {
						printf("read error");
						exit(1);
					} else if (ret == 0) {
						isClientwork = false;
					} else {
						//将消息发送给服务端
						ret = send(clie_fd, buf, strlen(buf), 0);
					}	
				}
			}
		}
	}
	if (pid == 0) {
		close(pipe_[1]);
	} else {
		close(pipe_[0]);
		close(clie_fd);
	}
	return 0;
}
