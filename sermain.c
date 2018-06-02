#include "server.h"//servermain.c

list users;
sqlite3* pdb;
const char* dbname = "chat.db";

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
	list_init(&users);
	//创建数据库 并创建用户注册表
	if(db_open(dbname,pdb) == -1)
		return -1;

	pthread_t tid0;
	int ret = pthread_create(&tid0,0,pexit,NULL);
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
	
		pthread_t tid;
		int t = pthread_create(&tid,0,pnewthread,(void*)&cfd);
		if(t != 0){
			printf("error %d: pthread_create failed.\n",t);
			return -1;
		}	
		printf(/*"%s: */"client thread cfd=%d created.\n",/*inet_ntop(AF_INET,&client.sin_addr,IP,32),*/cfd);
	}
	
	return 0;
}
