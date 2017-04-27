#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE		2048
#define OPEN_MAX	100
#define LISTENQ		20
#define SERV_PORT	5555
#define INFTIM		1000

void setnonblocking(int sock)
{
	int opts;
	opts = fcntl(sock, F_GETFL);
	if(opts < 0) {
		perror("fcntl(sock, GETFL)");
		exit(1);
	}
	opts = opts | O_NONBLOCK;
	if(fcntl(sock, F_SETFL, opts) < 0) {
		perror("fcntl(sock, SETFL, opts)");
		exit(1);
	}
}

int main(int argc, char *argv[])
{
	printf("epoll socket begins.\n");
	int i, serverfd, clientfd, sockfd, epfd, nfds;
	ssize_t n;
	
	socklen_t clilen;
	int yes = 1;
	char buf[BUFFER_SIZE+1];
	struct epoll_event ev, events[20];

	epfd = epoll_create(256);
	if(-1==epfd)
	{
		perror("epoll_create");
		return -1 ;
	}
	struct sockaddr_in clientaddr;
	struct sockaddr_in serveraddr;

	serverfd = socket(AF_INET, SOCK_STREAM, 0);
	if(-1==serverfd)
	{
		perror("socket");
		return -1 ;
	}
	setnonblocking(serverfd);

	ev.data.fd = serverfd;
	ev.events = EPOLLIN | EPOLLET;
	if(epoll_ctl(epfd, EPOLL_CTL_ADD, serverfd, &ev)<0)
	{
		perror("epoll_ctl");
		return -1 ;
	}

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;	// AF_UNSPEC
	serveraddr.sin_port = htons(SERV_PORT);
	serveraddr.sin_addr.s_addr = INADDR_ANY;

	if(setsockopt(serverfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
	{
		perror("setsockopt");
		exit(1);
	}

	if(bind(serverfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))<0)
	{
		perror("bind");
		return -1 ;
	}

	if(listen(serverfd, LISTENQ)<0)
	{
		perror("listen");
		return -1 ;
	}

	printf("epoll loop ...\n");
	clilen = sizeof(struct sockaddr_in);
	for(; ;) 
	{
		nfds = epoll_wait(epfd, events, 20, 500);
		if(nfds<0)	// error 
		{
			perror("epoll_wait error :");
			break ;
		}
		else if(0==nfds) // timeout
		{
			continue ;
		}
		//printf("[%s:%s:%d] nfds=%d\n", __FILE__,__func__, __LINE__,nfds);	
		for(i = 0; i < nfds; ++i) 
		{
			if(events[i].data.fd == serverfd) 
			{
				clientfd = accept(serverfd, (struct sockaddr *)&clientaddr, &clilen);
				if(clientfd < 0) 
				{
					perror("accept");
					exit(1);
				}
				setnonblocking(clientfd);
				char *str = inet_ntoa(clientaddr.sin_addr);
				printf("connect from client[%d]:%s\n",clientfd,str);

				ev.data.fd = clientfd;
				ev.events = EPOLLIN | EPOLLET;
				epoll_ctl(epfd, EPOLL_CTL_ADD, clientfd, &ev);
			}
			else if(events[i].events & EPOLLIN) 
			{
				if((sockfd = events[i].data.fd) < 0) 
					continue;
				//if((n = read(sockfd, buf, BUFFER_SIZE)) < 0) 
				n=recv(sockfd,buf,BUFFER_SIZE,0);
				if(n<0)
				{
					perror("recv");
					if(errno == ECONNRESET) 
					{
						printf("[%s:%s:%d] client[%d] close\n", __FILE__,__func__, __LINE__,sockfd);	
						close(sockfd);
						events[i].data.fd = -1;
					}
					continue ;
				}
				else if(n == 0) 
				{
					printf("[%s:%s:%d] client[%d] close\n", __FILE__,__func__, __LINE__,sockfd);
					close(sockfd);
					events[i].data.fd = -1;
					continue ;
				}
				buf[n]='\0';
				printf("[%s:%s:%d] client[%d] recv[%d]: %s\n",\
					__FILE__,__func__, __LINE__,sockfd,n,buf);

				ev.data.fd = sockfd;
				ev.events = EPOLLOUT | EPOLLET;
				epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev); //异步处理发送
				
			}
			else if(events[i].events & EPOLLOUT) 
			{
				sockfd = events[i].data.fd;
				write(sockfd, buf, n);

				printf("written data: %s\n", buf);

				ev.data.fd = sockfd;
				ev.events = EPOLLIN | EPOLLET;
				epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
			}
		}
	}
	close(serverfd);
	return 0 ;
}
