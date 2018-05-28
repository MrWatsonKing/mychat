#include "server.h"////////////////// server.c

extern list users;
extern sqlite3* pdb;
extern const char* dbname;

void* pexit(void* null){
	char cmd[32] = {0};
	while(1){
		fgets(cmd,32,stdin);//fgets()获取的字符串包含\n
		if(!strcmp(cmd,":exit\n"))
			exit(0);
	}
}

void* pnewthread(void* pcfd){
	
	char* myname = NULL;
	int cfd = *(int*)pcfd;
		
	while(1){
		switch(pcommand(cfd)){
			case LOGIN:
				plogin(cfd,&myname);
				break;
			case REGISTER:
				pregister(cfd);
				break;
			case CHECKON:
				pcheckon(cfd);
				break;
			case TALK:
				ptalk_transfer(cfd,myname);
				break;
			case QUIT:
				pquit(cfd);
				break;
			default:
				break;	
		}
	}
	return (void*)0;	
}

int pcommand(int cfd){
	
	char cmd[32] = {0};
	int n = 0;
	if((n = read(cfd,cmd,32)) < 0){
		perror("read error");
		return QUIT;//如果读不到command,就会发出退出命令
	}
	cmd[n] = '\0';

    if(!strcmp(cmd,"login\n"))
        return LOGIN;
    else if(!strcmp(cmd,"register\n"))
        return REGISTER;
    else if(!strcmp(cmd,"online\n"))
        return CHECKON;
    else if(!strcmp(cmd,"talk\n"))
        return TALK;
    else if(!strcmp(cmd,"sendfile\n"))
        return SENDFILE;
    else if(!strcmp(cmd,"quit\n"))
	        return QUIT;
		
	return QUIT;
}

int plogin(int cfd,char** pmyname){
	
	char buf[100] = {0};
	char username[32] = {0},password[32] = {0};

	int n = 0;
	if((n = read(cfd,buf,100)) < 0){
		printf("failed to read login message from client.\n");
		return -1;
	}
	buf[n] = '\0';
	sscanf(buf,"%s %s\n",username,password);

	switch(db_check(username,password,dbname,pdb)){
		case SQL_NONE:
			dprintf(cfd,"username or password wrong!\n");
//			printf("username or password wrong!\n");
			break;
		case SQL_FOUND:
			dprintf(cfd,"login successful!\n");
			*pmyname = (char*)malloc(32);
			strcpy(*pmyname,username);
//			printf("%s cfd=%d login successful!\n",username,cfd);
			break;
		case SQL_ERROR:
			dprintf(cfd,"database currently unavailable,please retry later!\n");
			break;
		default:
			break;			
	}
	return 0;
}

int pregister(int cfd){
	
	char buf[100] = {0};
	char username[32] = {0},password[32] = {0};

	int n = 0;
	if((n = read(cfd,buf,100)) < 0){
		printf("failed to read register message from client.\n");
		return -1;
	}
	buf[n] = '\0';
	sscanf(buf,"%s %s\n",username,password);
	if(!strcmp(username,"register") && !strcmp(password,"failed")){
//		printf("password inputs differ,client may retry.\n");
		return -1;
	}

	switch(db_check(username,password,dbname,pdb)){
		case SQL_NONE...SQL_FOUND:
			if(db_insert(username,password,dbname,pdb) == 0){
				if(db_check(username,password,dbname,pdb) == SQL_FOUND){
					dprintf(cfd,"user registered successfully!\n");
//					printf("user registered successfully!\n");
				}
			}else{
				dprintf(cfd,"username already exists,please re_register!\n");
//				printf("username already exists,please re_register!\n");
			}
			break;
		case SQL_ERROR:
			dprintf(cfd,"database currently unavailable,please retry later!\n");
			break;
		default:
			break;			
	}
	return 0;
}

int pcheckon(int cfd){
	
	list_show(&users,cfd);
	return 0;
}

int ptalk_transfer(int cfd,char* myname){
	
	if(list_getcfd(myname,&users) > 0){
		dprintf(cfd,"relogin: user is online somewhere else!\n");
		printf("relogin: user is online somewhere else.\n");
		return -1;
	}else{
		dprintf(cfd,"enter talkroom successful.");
//		printf("%d entered talkroom successful.\n",cfd);
	}

	char msg[1000] = {0};
	int tcfd = 0;
	char toname[32] = {0};
	list_append(myname,cfd,&users);

	int n = 0;
	int len = 0;
	while((n = read(cfd,msg,1000)) > 0){
		msg[n] = '\0';
		//msg自带\n,尤其是文件内容,不能删掉
		if(!strcmp(msg,"@. :exit\n")){
			char exitmsg[100] = {0};
			sprintf(exitmsg,"@. [msg]:left talk.\n");
			pgroupmsg(cfd,myname,exitmsg);
			list_delete(cfd,&users);
			break;
		}

		//所有消息格式都为@toname realmsg\n
		//组织成新的格式为fromname:@toname realmsg\n
		sscanf(msg,"@%s ",toname);
		len = strlen(toname);

		if(len == 1 && toname[0]=='.')//群发
			pgroupmsg(cfd,myname,msg);//包含@toname
		else{//单发
			if((tcfd = list_getcfd(toname,&users)) == -1){
				dprintf(cfd,"server:@%s @toname not online!\n",myname);
				continue;			
			}
			if(strstr(msg,":file") && strstr(msg,"$"))//只对文件命令发送确认消息
				dprintf(cfd,"server:@%s [verify]: OK.\n",myname);
			dprintf(tcfd,"%s:%s",myname,msg);//包含@toname
	//		printf("transfer realmsg len=%lu.\n",strlen(msg)-len-2);
		}
	}
	//printf("ptalk_transfer exited.\n");
	return 0;
}
void pgroupmsg(int mycfd,char* myname,char* msg){
	
	int clients = 0;
	int* cfdarr = NULL;
	char toname[32] = {0};
	sscanf(msg,"@%s ",toname);
	int len = strlen(toname);

	//该函数会调用malloc 所以用完之后 一定要free
	list_getcfdarr(&cfdarr,&clients,&users);
	
	for(int i=0; i<clients; i++)
		if(cfdarr[i] != mycfd){
			dprintf(cfdarr[i],"%s:%s",myname,msg);//包含@toname
	//		printf("broadcast realmsg len=%lu.\n",strlen(msg)-len-2);
		}
	
	//free 临时数组的内存
	free(cfdarr);
	cfdarr = NULL;
}

int pquit(int cfd){
	
	if(list_getname(cfd,&users))//如果cfd所对应的用户存在,则删除之
		list_delete(cfd,&users);//针对意外退出情况
	printf("client thread cfd=%d exited.\n",cfd);
	pthread_exit(NULL);
}

int plisten(int port,int backlog){
	
	SA4 serv;
	serv.sin_family = AF_INET;
	serv.sin_port = htons(port);
	serv.sin_addr.s_addr = htonl(INADDR_ANY);

	int sfd = socket(AF_INET,SOCK_STREAM,0);
	if(sfd == -1){
		perror("socket");
		return -1;
	}

	int b = bind(sfd,(SA*)&serv,sizeof(serv));
	if(b == -1){
		perror("bind");
		return -1;
	}

	int l = listen(sfd,backlog);
	if(l == -1){
		perror("listen");
		return -1;
	}
	
	return sfd;
}

