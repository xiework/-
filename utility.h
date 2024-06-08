#ifndef UTILITY_H_INCLUDED
#define UTILITY_H_INCLUDED
#include <iostream>
#include <list>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
using namespace std;

#define EPOLL_SIZE 2048
#define BUF_SIZE 2048
#define SERVER_PORT 5050
#define EXIT "exit"
#define WELCOMECLIENT "=====欢迎进入聊天室====="
list<int> cli_list;
//将fd挂到监听树上
void epoll_add(int epfd, int fd)
{
	//设置非阻塞
	int flag = fcntl(fd, F_GETFL);
	flag |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flag);

	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	int ret = epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
	if(ret < 0) {
		perror("epoll_ctl error");
		exit(1);
	}

}
//向其他客户端发送消息
int send_message(int cfd, char* buf, int flag) 
{
	int ret;
	char message[BUF_SIZE] = {0};
	if (flag == 0) {
		strncpy(message, buf, strlen(buf));
	} else if (flag == 1) {
		strncpy(message, buf, strlen(buf));	
		printf("客户端%d关闭\n", cfd);
		cli_list.remove(cfd);
		close(cfd);
	} else {
		sprintf(message, "客户端%d >> %s", cfd, buf);
	}
	if (cli_list.size() == 1) {
		printf("当前聊天室只有一个客户端\n");
		return 1;
	}

	//printf("%s\n",buf);
	sprintf(message, "客户端%d >> %s", cfd, buf);
	//向其他客户端发送消息
	for (list<int>::iterator  it = cli_list.begin(); it != cli_list.end(); ++it) {
		if (*it != cfd) {
			//printf("%s\n",message);
			ret = send(*it, message, strlen(message), 0);
			if (ret < 0) {
				perror("send error");
				exit(1);
			} else if (ret == 0) {
				printf("客户端%d关闭\n", *it);
			}
		}	
	}
	return 0;
}
#endif
