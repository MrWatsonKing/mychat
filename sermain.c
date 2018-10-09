#include "server.h"  // servermain.c
#include "threadpool.h"

list users;
sqlite3* pdb;
const char* dbname = "chat.db";
int nclients = 0;
int nthreads = 0;
int PORT = 8080;
int BACKLOG = 20;
int MAX_THRD_NUM = 5000;
int efd = 0;
threadpool_t* pool;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

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
	efd = epoll_create1(0);
	if(efd == -1){
		perror("epoll_create1");
		return -1;
	}

	//注册epoll事件
	struct epoll_event ev,evq[BACKLOG];	
	ev.events = EPOLLIN|EPOLLOUT;
	ev.data.fd = sfd;	
	epoll_ctl(efd,EPOLL_CTL_ADD,sfd,&ev);

	//创建线程池
	//注意：一个线程占用内存较多 所以一个进程所能够支持的最大的线程数量 是有限制的。
	//设置setstacksize = 1M. 这样5G内存可以容纳的最大线程数 就可能达到5000
	//本机安装的是64位操作系统 理论上是可以使用超过5G内存空间的
	pool = threadpool_create(10,MAX_THRD_NUM,BACKLOG);
	if(pool == NULL) return -1;

	//开始监听来自客户端的连接
	int l = listen(sfd,BACKLOG);
	if(l == -1){
		perror("listen");
		return -1;
	}
	printf("start listening ...\n");


	int nfds=0,i=0,n=0;
	SA4 client;
	socklen_t clilen = sizeof(client);

	//等待epoll通知 处理epoll消息
	while(1){
		nfds = epoll_wait(efd,evq,BACKLOG,100); //-1无限等待 0 立即返回 n>0等待毫秒数
		
		//遍历产生新消息的通知队列
		//在单线程的情况下 就是依次处理 不能做到并发 并发问题稍后解决
		for(i=0;i<nfds;i++){
			//如果产生消息的套接字 是原始监听套接字 则创建新的套接字
			if(evq[i].data.fd == sfd){
				//接收新的连接请求 创建新的连接套接字
				int cfd = accept(sfd,(SA*)&client,&clilen);
				if(cfd == -1){
					perror("accept");
					return -1;
				}
				//将新的连接套接字 注册到epoll对象中
				ev.data.fd = cfd;
				ev.events = EPOLLIN|EPOLLET; //边缘触发 读操作
				epoll_ctl(efd,EPOLL_CTL_ADD,cfd,&ev);

				if(!list_append(cfd)){
					pthread_mutex_lock(&mtx);
					nclients++;
					pthread_mutex_unlock(&mtx);
					printf("client cfd=%d added.\ttotal clients: %d\n",cfd,nclients);
				}				
			
			//如果产生消息的套接字 是原始监听套接字之外的其他连接套接字 则处理连接消息
			}else if(evq[i].events & EPOLLIN){ //处理读消息
				// printf("EPOLLIN: new message from client:\n");
				//根据客户消息内容 进行针对性的操作
				//get commands:
				int cfd = evq[i].data.fd;
				char cmd[32] = {0};
				ssize_t n = 0;
				if((n = read(cfd,cmd,32)) < 0){
					perror("read error");
					//如果读不到command,表示客户连接断线，将从list删除客户，并跳出循环结束本服务线程
					list_exit(cfd);
					break;
				}
				cmd[n] = '\0';
				// printf("%s",cmd);

				//把各个函数环节 打包成task 通过线程池来并行处理
				//注意：if判断结束的时候 局部变量cfd会被销毁 
				//所以不可以用指针传递的方式传参 只能用值传递的方式传入cfd。
				if(!strcmp(cmd,"login\n"))
					threadpool_add_task(pool,plogin,(void*)cfd);
				else if(!strcmp(cmd,"logout\n"))
					threadpool_add_task(pool,plogout,(void*)cfd);
				else if(!strcmp(cmd,"register\n"))
					threadpool_add_task(pool,pregister,(void*)cfd);
				else if(!strcmp(cmd,"online\n"))
					threadpool_add_task(pool,pcheckon,(void*)cfd);
				else if(!strcmp(cmd,"talk\n"))
					threadpool_add_task(pool,ptalk_transfer,(void*)cfd);
				else if(!strcmp(cmd,"shares\n"))
					threadpool_add_task(pool,pcheckfiles,(void*)cfd);
				else if(!strcmp(cmd,"quit\n"))
					threadpool_add_task(pool,pquit,(void*)cfd);

			}else if(evq[i].events & EPOLLOUT){ //处理写消息
				printf("EPOLLOUT: ready to write to client:\n");
			}
			//其他类型暂不接收 也不处理
		}
	}

	return 0;
}
