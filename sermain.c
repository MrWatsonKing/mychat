#include "server.h"  // servermain.c

list users;
sqlite3* pdb;
const char* dbname = "chat.db";
int nclient = 0;
int nthreads = 0;

int main(int argc,char** argv){
	SA4 client;
	socklen_t clilen = sizeof(client);

	int sfd = plisten(8080,6);
	if(sfd == -1){
		printf("plisten failed.\n");
		return -1;
	}
	printf("start listening ...\n");
	//初始化在线用户链表
	list_init();
	//创建数据库 并创建用户注册表
	if(db_open(dbname,pdb) == -1)
		return -1;

	//服务器运行界面控制命令
	pthread_t tid0;
	int ret = pthread_create(&tid0,0,pcontrol,NULL);
	if(ret != 0){
		printf("error %d: pthread_create failed.\n",ret);
		return -1;
	}

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
		// for(int i=nclient;i<1000;i++){
		// 	if(ctdarr[i].status == 0){
		// 		ctdarr[i].cfd = cfd;
		// 		ctdarr[i].tid = tid;
		// 		ctdarr[i].status = 1;
		// 		nclient++;
		// 		nthreads++;
		// 		printf(/*"%s: */"client thread cfd=%d created.\ttotal threads: %d\n",/*inet_ntop(AF_INET,&client.sin_addr,IP,32),*/cfd,nthreads);
		// 		break;
		// 	}			
		// }
		// if(nclient == 1000) nclient = 0;	
		if(!list_append(cfd,tid))
			printf(/*"%s: */"client thread cfd=%d created.\ttotal threads: %d\n",/*inet_ntop(AF_INET,&client.sin_addr,IP,32),*/cfd,nthreads);	
	}	

	return 0;
}
