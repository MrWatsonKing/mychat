#include "server.h"  // servermain.c

list users;
sqlite3* pdb;
const char* dbname = "chat.db";
int nclients = 0;
int nthreads = 0;
int PORT = 8080;
int BACKLOG = 20;

int main(int argc,char** argv){
	//服务器运行界面命令控制线程
	pthread_t tid0;
	int ret = pthread_create(&tid0,0,pcontrol,NULL);
	if(ret != 0){
		printf("error %d: pthread_create failed.\n",ret);
		return -1;
	}

	//初始化在线用户链表
	list_init();
	//创建数据库 并创建用户注册表
	if(db_open(dbname,pdb) == -1)
		return -1;	
	
	//创建并绑定套接字到服务地址和端口
	int sfd = pbind(PORT);
	if(sfd == -1){
		printf("pbind failed.\n");
		return -1;
	}
	//设置非阻塞标签
	setnonblock(sfd);

	//创建epoll对象
	int efd = epoll_create1(0);
	if(efd == -1){
		perror("epoll_create1");
		return -1;
	}

	//注册epoll事件
	struct epoll_event ev,evq[BACKLOG];	
	ev.events = EPOLLIN|EPOLLOUT;
	ev.data.fd = sfd;	
	epoll_ctl(efd,EPOLL_CTL_ADD,sfd,&ev);

	//开始监听来自客户端的连接
	int l = listen(sfd,BACKLOG);
	if(l == -1){
		perror("listen");
		return -1;
	}
	printf("start listening ...\n");


	int nfds=0,i=0,cfd=0;
	SA4 client;
	socklen_t clilen = sizeof(client);

	//等待epoll通知 处理epoll消息
	while(1){
		nfds = epoll_wait(efd,evq,BACKLOG,100); //-1无限等待 0 立即返回 n>0等待毫秒数
		//遍历产生新消息的通知队列
		for(i=0;i<nfds;i++){
			//如果产生消息的套接字 是原始监听套接字 则创建新的套接字
			if(evq[i].data.fd == sfd){
				//接收新的连接请求 创建新的连接套接字
				cfd = accept(sfd,(SA*)&client,&clilen);
				if(cfd == -1){
					perror("accept");
					return -1;
				}
				//将新的连接套接字 注册到epoll对象中
				ev.data.fd = cfd;
				ev.events = EPOLLIN|EPOLLET; //边缘触发 读操作
				epoll_ctl(efd,EPOLL_CTL_ADD,cfd,&ev);
				nclients++;
				printf(/*"%s: */"client cfd=%d added.\ttotal clients: %d\n",cfd,nclients);
			
			//如果产生消息的套接字 是原始监听套接字之外的其他连接套接字 则处理连接消息
			}else if(evq[i].events & EPOLLIN){ //处理读消息
				printf("EPOLLIN: new message from client:\n");
				//根据客户消息内容 进行针对性的操作

			}else if(evq[i].events & EPOLLOUT){ //处理写消息
				printf("EPOLLOUT: ready to write to client:\n");
			}
			//其他类型暂不接收 也不处理
		}
	}

#if 0		
	//循环接收客户端连接套接字
	while(1){
		//char IP[32] = {0};
		int cfd = accept(sfd,(SA*)&client,&clilen);
		if(cfd == -1){
			perror("accept");
			return -1;
		}

		//每一个新的客户端连接进来，都会生成一个对应的服务线程，来提供专门的服务
		pthread_t tid;
		int t = pthread_create(&tid,0,pnewthread,(void*)&cfd);
		if(t != 0){
			printf("error %d: pthread_create failed.\n",t);
			return -1;
		}	

		if(!list_append(cfd,tid))
			printf(/*"%s: */"client thread cfd=%d created.\ttotal threads: %d\n",/*inet_ntop(AF_INET,&client.sin_addr,IP,32),*/cfd,nthreads);	
	}	
#endif

	return 0;
}
