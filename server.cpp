#include "utility.h"


//创建监听socket
int create_socket_lfd()
{
	//1.创建socket lfd;
	int lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1) {
		perror("socket error");
		exit(1);
	}
	//端口复用
	int opt = -1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, (void*)&opt, sizeof(opt));
	//2.绑定地址结构
	sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(SERVER_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	int ret = bind(lfd, (sockaddr*)&server_addr, sizeof(server_addr));
	if (ret == -1) {
		perror("bind error");
		exit(1);
	}

	//3.设置监听上限
	ret = listen(lfd, 128);
	if (ret == -1){
		perror("listen error");
		exit(1);
	}
	return lfd;
}
//创建客户端连接
int my_accept(int lfd)
{
	sockaddr_in client_addr;
	socklen_t client_addr_len = sizeof(client_addr);
	int cfd = accept(lfd, (sockaddr*)&client_addr, &client_addr_len);
	if (cfd == -1) {
		perror("accept error");
		exit(1);
	}
	return cfd;
}
int main()
{
	int ret;
	//1.创建监听套接字lfd
	int lfd = create_socket_lfd();
	//2.创建监听树
	int epfd = epoll_create(EPOLL_SIZE);
	if (epfd == -1) {
		perror("epoll_create error");
		exit(1);
	}
	//3.将lfd加入监听树
	epoll_add(epfd, lfd);
	//4.监听事件
	epoll_event events[EPOLL_SIZE];
	//接收缓冲区
	char mbuf[BUF_SIZE] = {0};
	while(1) {
		//监听事件
		int event_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
		//printf("事件产生\n");
		//处理事件
		for (int i = 0; i < event_count; i++) {
			bzero(mbuf, sizeof(mbuf));
			epoll_event cli_ev = events[i];
			if (cli_ev.data.fd == lfd) {//有新的客户端连接上来
				int cfd = my_accept(lfd);//与客户端连接
				//printf("客户端建立连接");
				//将cfd加入监听树
				epoll_add(epfd, cfd);
				//将cfd加入到集合中
				cli_list.push_back(cfd);
				ret = send(cfd, WELCOMECLIENT, strlen(WELCOMECLIENT), 0);
				if (ret == -1) {
					perror("send error");
					exit(1);
				}
				sprintf(mbuf, "=====用户%d进入聊天室=====", cfd);
				ret = send_message(cfd, mbuf, 0);	
			} else {//有客户端发送消息
				int ret = recv(cli_ev.data.fd, mbuf, sizeof(mbuf), 0);
				if (ret == -1) {
					perror("recv error");
					exit(1);
				} else if (ret == 0) {
					sprintf(mbuf, "=====用户%d退出聊天室=====", cli_ev.data.fd);
					ret = send_message(cli_ev.data.fd, mbuf, 1);
				} else {
					ret = send_message(cli_ev.data.fd, mbuf, 2);
				}
			}
		}
	}
	close(lfd);
	close(epfd);
	return 0;
}
