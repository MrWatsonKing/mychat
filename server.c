#include "server.h"////////////////// server.c

extern int nthreads;
extern list users;
extern sqlite3* pdb;

void* pexit(void* null){
	char cmd[32] = {0};
	while(1){
		fgets(cmd,32,stdin); //fgets()获取的字符串包含\n
		if(!strcmp(cmd,":online\n")){
			int cnt = list_count("chaters");
			printf("chaters online: %d\n",cnt);
			if(cnt>0){
				char names[32*cnt];
				printf("%s [over]\n\n",list_names(cnt,names));
			}else
				printf("\n");			
		}			
		if(!strcmp(cmd,":exit\n")){
			list_destroy();			
			exit(0);
		}							
	}
}

void* pnewthread(void* pcfd){
	
	int cfd = *(int*)pcfd;
	//myname必须是一个线程内的局部变量，因为不同的线程，对应不同的myname
	char myname[32] = {0};	
	
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
            plogin(cfd,myname); 
		else if(!strcmp(cmd,"logout\n"))
            plogout(cfd,myname);
        else if(!strcmp(cmd,"register\n"))
            pregister(cfd);
        else if(!strcmp(cmd,"online\n"))
            pcheckon(cfd);
        else if(!strcmp(cmd,"talk\n"))
            ptalk_transfer(cfd,myname);
        else if(!strcmp(cmd,"quit\n")){
			list_exit(cfd);
			break;
		}           
	}
	printf("client thread cfd=%d exited.\ttotal threads: %d\n",cfd,nthreads);
	return (void*)0;	
}

int plogin(int cfd,char* myname){
	
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
    
	switch(db_check(username,password)){
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
				strcpy(myname,username);		
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

int plogout(int cfd,char* myname){
	
	if(!list_logout(cfd)){
		memset(myname,0,32);
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

	switch(db_check(username,password)){
		case SQL_NONE...SQL_FOUND:
			if(db_insert(username,password) == 0){
				if(db_check(username,password) == SQL_FOUND){
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
        
    int cnt = list_count("chaters");
	//临近的代码区间内，尽量将通信语句一次性发送，因为发送和接收的次数实际上并不是匹配的
	//连续快速的发送，可能会被接收端视为同一次发送，从而不能正常解析单词发送的通信语句
	if(cnt > 0)
	    dprintf(cfd, "server:@. chaters online: %d\n",cnt);
    else{
		dprintf(cfd, "server:@. chaters online: %d\n\n",cnt);
		return 0; 
	}		   
    
    char names[32*cnt];
    list_names(cnt,names);
    
	//分段发送，是为了让同样的代码既可以和command:online对接，也可以和chatroom中:online对接
    int ncuts = strlen(names)/900+1;
    char temp[1000] = {0};

	//多次发送之间，都进行了字符串拷贝操作，造成了一定的时间间隔，
	//才能使得客户端的read()函数能够正确的解析每一次发送的数据，而不是一次性获取多条数据
    for(int i=0;i<ncuts;i++){
        if(i < ncuts -1){	//不是最后一次
            memcpy(temp,names+i*900,900);
            temp[900] = '\0';
			dprintf(cfd,"server:@. %s",temp);
        }else{	//最后一次
			strcpy(temp, names+i*900);
			dprintf(cfd,"server:@. %s [over]\n\n",temp);
		}
    }
    
	return cnt;
}

int ptalk_transfer(int cfd,char* myname){
	
	if(!list_chatin(cfd))
		dprintf(cfd,"enter talkroom successful.\n");
	else{
		dprintf(cfd,"enter talkroom failure.\n");
		return -1;
	}

	char msg[1000] = {0};
	char filepath[256] = {0};
	int tcfd = 0;
	char toname[32] = {0};
	char tmptoname[32] = {0};	
    int lento = 0;
	ssize_t n = 0;
	int online = 0;	
    
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
			pgroupmsg(cfd,exitmsg,myname);
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
		lento = (int)strlen(toname);

		//群发消息 文件上传下载
		if(lento == 1 && toname[0]=='.') {
			if(strstr(msg,":upload")){
				sscanf(msg,"%*[^$]$%s",filepath);
				strtok(filepath,"\n");
				pfile_upload(cfd,filepath);
			}
			else if(strstr(msg,":download")){
				sscanf(msg,"%*[^$]$%s",filepath);
				strtok(filepath,"\n");
				pfile_download(cfd,filepath);
			}
			else 
				pgroupmsg(cfd,msg,myname); //群发消息 包含@toname
		//单发消息 文件定向传送
		}else{
			//如果本次目标用户名同上次不一样 就重新设定在线状态
			if(strcmp(toname,tmptoname)){
				online = 0;
				strcpy(tmptoname,toname);
			}
			//进行目标用户在线状态验证
			if(online == 0){
				//printf("checking @toname online status:\n");
				if((tcfd = list_getcfd(toname)) == -1){
					online = 0;
					dprintf(cfd,"server:@. [verify]: NOL.\n");
					continue;			
				}
				dprintf(cfd,"server:@. [verify]: OL.\n");
				online = 1;
			}				
			//验证通过之后进行消息转发	
			dprintf(tcfd,"%s:%s",myname,msg); //包含@toname
	//		printf("transfer realmsg len=%lu.\n",strlen(msg)-len-2);
		}
	}
	//printf("ptalk_transfer exited.\n");
	return 0;
}
void pgroupmsg(int mycfd,char* msg,char* myname){
	
	int clients = 0;
	int* cfdarr = NULL;
	char toname[32] = {0};
	sscanf(msg,"@%s ",toname);

	//该函数会调用malloc 所以用完之后 一定要free
	list_getcfdarr(&cfdarr,&clients);
	
	for(int i=0; i<clients; i++)
		if(cfdarr[i] != mycfd){
			dprintf(cfdarr[i],"%s:%s",myname,msg); //包含@toname
	//		printf("broadcast realmsg len=%lu.\n",strlen(msg)-len-2);
		}
	
	//free 临时数组的内存
	free(cfdarr);
	cfdarr = NULL;
}

void pfile_upload(int cfd,char* filepath){
	printf("\nfile_upload: %s\n",filepath);

	//获取文件名
	char* filename = NULL;
	if(strstr(filepath,"/"))
		filename = 1 + strrchr(filepath,'/');
	else
		filename = filepath;
	printf("name=%s\n",filename);

	//获取文件大小
	int size = 0;
	char sizebuf[64] = {0};
	int n = 0;
	if((n = read(cfd,sizebuf,64)) < 0){
		perror("read error");
		printf("\n");
		return;
	}
	sizebuf[n] = '\0';
	if(strstr(sizebuf,"$staterr$") || strstr(sizebuf,"$sizeerr$")){
		printf("sender failed to fetch file size.\n\n");
		return;
	}
	//size > 0，才会接收到文件大小
	sscanf(sizebuf,"%*s filesize=%d\n",&size);
//	printf("size=%d\n",size);

	//创建接收文件的文件夹
    char cwd[100] = {0};
    char recvpath[256] = {0};
    getcwd(cwd,100);
	sprintf(recvpath,"%s%s",cwd,"/shared_files/");

    if(access(recvpath,R_OK|W_OK|X_OK) == -1){
        if(mkdir(recvpath,0777) == -1){
            dprintf(cfd,"server:@. [verify]: NO.\n");
            perror("mkdir error");
            return;
        }else
			printf("dir created OK:%s\n",recvpath);
    }	
    strcat(recvpath,filename);
	dprintf(cfd,"server:@. [verify]: OK.\n");

	//验证发送方文件是否打开成功
	if((n = read(cfd,sizebuf,64)) < 0){
		perror("read error");
		printf("\n");
		return;
	}
	sizebuf[n] = '\0';
	if(strstr(sizebuf,"$openerr$")){
		printf("error: sender failed to open file.\n");
		return;
	}

    FILE* precvfile = fopen(recvpath,"w");
	if(precvfile == NULL){
		dprintf(cfd,"server:@. [verify]: SS.\n");
		perror("fopen error");
		fclose(precvfile);
		printf("\n");
		return;
	}

	int r = 0;
	int w = 0;
	int wsum = 0;
	char filebuf[1000] = {0};
	char realmsg[1000] = {0};
	int lenreal = 0;
	//因为read()返回次数不确定，所以循环次数不可以与发送次数一致
	while(1){
		//通知发送方可以发送了
		dprintf(cfd,"server:@. [verify]: CC.\n");

		r = read(cfd,filebuf,1000); //首先进入等待状态,阻塞接收
		if(r < 0){//格式为@. realmsg\n
			dprintf(cfd,"server:@. [verify]: SS.\n");
			perror("read error");
			fclose(precvfile);
			precvfile = NULL;
			printf("\n");
			return;
		}
		filebuf[r] = '\0';
		//一共接收r个有效字符,
		//格式为@. realmsg\n
		strcpy(realmsg,filebuf+3);
		//绝对不能用sscanf(),因为它遇空格或者换行就会停止

		if(strstr(realmsg,"$readerr$")){			
			printf("sender failed to send file content.\n\n");
			fclose(precvfile);
			precvfile = NULL;
			return;
		}
		lenreal = strlen(realmsg); //包含\n
		realmsg[lenreal-1] = '\0'; // \n替换为\0

		if((w = fwrite(realmsg,1,strlen(realmsg),precvfile)) < 0){
			dprintf(cfd,"server:@. [verify]: SS.\n");
			ferror(precvfile);
			fclose(precvfile);
			precvfile = NULL;
			printf("\n");
			return;
		}

		wsum += w;
		printf("recved: %d bytes, %%%.2lf...\n",w,wsum*100.0/size);
		if(wsum >= size) break;
	}

	fclose(precvfile);
	precvfile = NULL;
	printf("file size=%d recved successful.\n\n",size);

}

void pfile_download(int cfd,char* filepath){
	printf("\nfile_download: %s\n",filepath);
	dprintf(cfd,"server:@. :download $%s\n",filepath);	

	//获取文件名
	char* filename = NULL;
	if(strstr(filepath,"/"))
		filename = 1 + strrchr(filepath,'/');
	else
		filename = filepath;
	printf("name=%s\n",filename);

	char path[256] = {0};
	char cwd[100] = {0};
	getcwd(cwd,100);
	sprintf(path,"%s%s%s",cwd,"/shared_files/",filename);

	//获取并发送文件大小
	int size = 0;
	struct stat filestat = {0};
	if(stat(path,&filestat) == -1){
		dprintf(cfd,"server:@. $staterr$\n");
		perror("stat error");
		printf("\n");
		return;
	}
	size = filestat.st_size;
	if(size == 0){
		dprintf(cfd,"server:@. $sizeerr$\n");
		printf("filesize=0,failed to send file.\n\n");
		return;
	}
	//size > 0，则发送文件大小	
	dprintf(cfd,"server:@. filesize=%d\n",size);

	char sizebuf[64] = {0};
	int n = 0;
	if((n = read(cfd,sizebuf,64)) < 0){
		perror("read error");
		printf("\n");
		return;
	}
	sizebuf[n] = '\0';
	if(strstr(sizebuf,"[verify]: NO.")){
		printf("client canceled file_download action.\n\n");
		return;
	}
	
	//打开本地文件，并根据结果向服务器发送进展情况
	FILE* psendfile = fopen(path,"r");
	if(psendfile == NULL){
		dprintf(cfd,"server:@. $openerr$\n");
		perror("fopen error");
		fclose(psendfile);
		printf("\n");
		return;
	}
	dprintf(cfd,"server:@. fileopen OK.\n");

	int m = 0,w = 0;
	int wsum = 0;
	char filebuf[900] = {0}; //不超过服务器接收范围
	while(1){
		if((n = read(cfd,sizebuf,64)) < 0){
			perror("read error");
			printf("\n");
			return;
		}
		sizebuf[n] = '\0';
		if(!strstr(sizebuf,"[verify]: CC.")){
			printf("client canceled file_download action.\n\n");
			return;
		}

		if((m = fread(filebuf,1,900,psendfile)) < 0){
			dprintf(cfd,"@. $readerr$\n");
			ferror(psendfile);
			printf("\n");
			fclose(psendfile);
			psendfile = NULL;
			return;
		}
		filebuf[m] = '\0';

		w = dprintf(cfd,"@. %s\n", filebuf); //增加\n以出尽缓存

		wsum += w-4;
		printf("sent: %d bytes, %%%.2lf...\n",w-4,wsum*100.0/size);				
		if(wsum >= size)  break;
	}
		
	fclose(psendfile);
	psendfile = NULL;
	printf("file size=%d sent successful.\n\n",size);

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

