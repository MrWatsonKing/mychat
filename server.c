#include "server.h"////////////////// server.c

extern int nthreads;
extern list users;
extern sqlite3* pdb;
extern const char* dbname;
char myname[32] = {0};

void* pexit(void* null){
	char cmd[32] = {0};
	while(1){
		fgets(cmd,32,stdin);//fgets()获取的字符串包含\n
		if(!strcmp(cmd,":online\n"))
			list_show();
		if(!strcmp(cmd,":exit\n")){			
			free(pdb);			
			pdb = NULL;
			exit(0);
		}							
	}
}

void* pnewthread(void* pcfd){
	
	int cfd = *(int*)pcfd;	
	
	while(1){
        //get commands:
        char cmd[32] = {0};
        ssize_t n = 0;
        if((n = read(cfd,cmd,32)) < 0){
            perror("read error");
			//如果读不到command,就会发出退出命令
            list_exit(cfd);
			break;
        }
        cmd[n] = '\0';
        // printf("%s",cmd);

        //execute commands:
        if(!strcmp(cmd,"login\n"))
            plogin(cfd); 
		else if(!strcmp(cmd,"logout\n"))
            plogout(cfd);
        else if(!strcmp(cmd,"register\n"))
            pregister(cfd);
        else if(!strcmp(cmd,"online\n"))
            pcheckon(cfd);
        else if(!strcmp(cmd,"talk\n"))
            ptalk_transfer(cfd);
        else if(!strcmp(cmd,"quit\n")){
			list_exit(cfd);
			break;
		}           
	}
	printf("client thread cfd=%d exited.\ttotal threads: %d\n",cfd,nthreads);
	return (void*)0;	
}

int plogin(int cfd){
	
	char buf[100] = {0};
    int tcfd = 0;
	char username[32] = {0},password[32] = {0};

	ssize_t n = 0;
	if((n = read(cfd,buf,100)) < 0){
		printf("failed to read login message from client.\n");
		return -1;
	}
	buf[n] = '\0';
	sscanf(buf,"%s %s\n",username,password);
    
	switch(db_check(username,password,dbname,pdb)){
		case SQL_NONE:
			dprintf(cfd,"username or password wrong!\n");
			break;
		case SQL_FOUND:
            //如果登陆账号密码都正确，但发现用户名已经在userlist当中，则默认挤掉
            //这种情况有两种可能，一种是用户正常的重复登录行为，
			//一种是客户端崩溃导致服务器没有及时删除用户在线信息
			//只要能够获取用户名，就表明已经正常登录，且logstatus==1
            if((tcfd = list_getcfd(username)) > 0){ 
				//回收线程，并将节点从用户列表删除
                list_delete(tcfd);
				//发消息给可能仍在运行的客户端，使其发出提示并强制下线
                dprintf(tcfd,"server:@. [verify]: QUIT.\n");				
            }			
			//需要检查当前登录 -1用户线程不存在 0用户线程存在但未登录 1用户线程正常登录 
			if(list_logstatus(cfd) == 1){
				dprintf(cfd,"already login! For relogin,try command:logout first.\n");
				break;
			}			
			if(!list_login(cfd,username)){		
				dprintf(cfd,"login successful!\n");
				return 0;
			}
			dprintf(cfd,"login failure.\n");
			break;
		case SQL_ERROR:
			dprintf(cfd,"database currently unavailable,please retry later!\n");
			break;
		default:
			break;			
	}
	return -1;
}

int plogout(int cfd){
	
	if(!list_logout(cfd)){
		dprintf(cfd,"logout successful!\n");
		return 0;
	}
	dprintf(cfd,"logout failure!\n");
	return -1;
}

int pregister(int cfd){
	
	char buf[100] = {0};
	char username[32] = {0},password[32] = {0};

	ssize_t n = 0;
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
    //获取最新时间
    time_t t = 0;
    struct tm *today = NULL;
    t = time(NULL);
    today = localtime(&t);
    
    int chaters = list_count("chaters");
    dprintf(cfd, "server:@. chaters online: %d\n",chaters);
    if(chaters == 0){
        dprintf(cfd,"server:@. [%02d:%02d:%02d]\n",today->tm_hour,today->tm_min,today->tm_sec);
        return 0;
    }
    
    char* userlist= (char*)malloc(32*chaters);
    if(userlist == NULL){
        dprintf(cfd,"server:@. failed to get userlist.\n");
        //printf("failed to malloc for userlist.\n");
        return -1;
    }
    userlist[0] = '\0';

    int i = 0;
    node* pnode = NULL;
    int lensum = 0;	
    //如果plist->head.pnext == &plist->tail,即plist当中没有有效成员的话,就不会进行循环
    for(pnode = users.head.pnext; pnode != &users.tail; pnode = pnode->pnext){
		if(i<chaters){
			strcat(userlist,pnode->username);
			strcat(userlist," ");
			lensum += strlen(pnode->username)+1;
			i++;
		}
    }
    userlist[lensum] = '\0';
    
    int ncuts = lensum/900+1;
    char temp[1000] = {0};
    for(int i=0;i<ncuts;i++){
        if(i < ncuts -1){
            memcpy(temp,userlist+i*900,900);
            temp[900] = '\0';
        }else
            strcpy(temp, userlist+i*900);
        
        dprintf(cfd,"server:@. %s",temp);
    }
    dprintf(cfd,"server:@. [%02d:%02d:%02d]\n",today->tm_hour,today->tm_min,today->tm_sec);
    
    free(userlist);
    userlist = NULL;
	
	return chaters;
}

int ptalk_transfer(int cfd){
	
	if(!list_chatin(cfd))
		dprintf(cfd,"enter talkroom successful.\n");
	else{
		dprintf(cfd,"enter talkroom failure.\n");
		return -1;
	}

	char msg[1000] = {0};
	int tcfd = 0;
	char toname[32] = {0};	
    int len = 0;
	ssize_t n = 0;	
    
	while((n = read(cfd,msg,1000)) > 0){
        //msg自带\n,尤其是文件内容,不能删掉
		msg[n] = '\0';
        
        if(!strcmp(msg,"@. :online\n")){
            pcheckon(cfd);
            continue;
        }        
		if(!strcmp(msg,"@. :exit\n")){
			char exitmsg[100] = {0};
			sprintf(exitmsg,"@. [msg]:left chatroom.\n");
			pgroupmsg(cfd,exitmsg);
			list_chatout(cfd);			
			break;
		}
		if(!strcmp(msg,"@. :quit\n")){
			list_chatout(cfd);
			break;
		}

		//所有消息格式都为@toname realmsg\n
		//组织成新的格式为fromname:@toname realmsg\n
		sscanf(msg,"@%s ",toname);
		len = (int)strlen(toname);

		if(len == 1 && toname[0]=='.')//群发
			pgroupmsg(cfd,msg);//包含@toname
		else{//单发
			if((tcfd = list_getcfd(toname)) == -1){
				dprintf(cfd,"server:@. @toname is not online!\n");
				continue;			
			}
			if(strstr(msg,":file") && strstr(msg,"$"))//只对文件命令发送确认消息
				dprintf(cfd,"server:@. [verify]: OK.\n");
			dprintf(tcfd,"%s:%s",myname,msg);//包含@toname
	//		printf("transfer realmsg len=%lu.\n",strlen(msg)-len-2);
		}
	}
	//printf("ptalk_transfer exited.\n");
	return 0;
}
void pgroupmsg(int mycfd,char* msg){
	
	int clients = 0;
	int* cfdarr = NULL;
	char toname[32] = {0};
	sscanf(msg,"@%s ",toname);

	//该函数会调用malloc 所以用完之后 一定要free
	list_getcfdarr(&cfdarr,&clients);
	
	for(int i=0; i<clients; i++)
		if(cfdarr[i] != mycfd){
			dprintf(cfdarr[i],"%s:%s",myname,msg);//包含@toname
	//		printf("broadcast realmsg len=%lu.\n",strlen(msg)-len-2);
		}
	
	//free 临时数组的内存
	free(cfdarr);
	cfdarr = NULL;
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

