 #include "client.h" //client.c

extern char cmd[32];
extern int logstatus;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int ncond = 0;
FILE* pfile = NULL;
char myname[32] = {0};

void phelp(void){

    printf("commands:\n"
           "\tlogin\t\tset logstatus on;\n"
           "\tlogout\t\tset logstatus off;\n"
           "\tregister\tregister user ID;\n"
           "\tonline\t\tcheck online userlist;\n"
		   "\tfiles\t\tcheck online filelist;\n"
           "\ttalk\t\tenter talkroom;\n"
           "\tquit\t\tquit this client.\n\n"
           
		   "chatting:\n"
		   "\t\":help\" to get helplist;\n"
           "\t\"@<name> <msg>\" to send a private msg;\n"
           "\t\"<msg>\" to broatcast a public msg;\n"
           "\t\":exit\" to exit chatroom;\n"
		   "\t\":online\" to check online userlist.\n"
		   "\t\":files\" to check online filelist.\n\n" 

           "file sharing:\n"
           "\t\"@<name> :file $<filepath>\" to send a file privately;\n"
           "\t\":upload $<filepath>\" to upload a file to server;\n"
		   "\t\":download $<filename>\" to download a file from server.\n\n"
           );  
}

int psendcmd(int sfd){

	strtok(cmd,"\n");
	dprintf(sfd,"%s\n",cmd);
//	printf("command sent: %s\n",cmd);
	return 0;
}

//注意:logstatus是存储在客户端本地的登录状态判断条件，
//其状态是其他有关功能开启或终止的前提条件，
//但服务器仅将登入chatroom的用户计入online list，
//以便更有针对性的统计处于聊天状态的实时有效用户。
//plogin会通过服务器进行对比验证，但服务器不存储其登录状态
//plogin成功后，logstatus置为1，否则logstatus等于初值0
//plogout将logstatus重新置为0。

int plogin(int sfd){

	char buf[100] = {0};
	char username[32],password[32];
	while(1){
		printf("username:");
		fgets(username,32,stdin); //包含'\n'
		if(strchr(username,' ')){
			printf("space is not permitted in username.\n");
			continue;
		}
		if(!strcmp(username,"\n")){
			continue;
		}
		if(strlen(username) > 24){
			printf("username should be <= 24 characters\n");
			continue;
		}
		break;
	}
	
	while(1){
		strcpy(password,getpass("password:")); //包含'\n'
		if(strchr(password,' ')){
			printf("space is not permitted in password.\n");
			continue;
		}
		if(!strcmp(password,"\n")){
			continue;
		}
		if(strlen(password) > 24){
			printf("password should be <= 24 characters.\n");
			continue;
		}
		break;
	}
	strtok(username,"\n");
	strtok(password,"\n");
	dprintf(sfd,"%s %s\n",username,password);
	//全局变量myname赋值
	strcpy(myname,username);
	
	int n = 0;
	if((n = read(sfd,buf,100)) < 0 ){
        printf("failed to read login reply from server.\n\n");
		return -1;
	}
	buf[n] = '\0';

	if(strstr(buf,"successful")){
		logstatus = 1;
        printf("login successful!\n\n");
		return 0;
	}
        
	printf("%s\n",buf);
	return -1;
}

int plogout(int sfd){

	char buf[100] = {0};
	int n = 0;
	if((n = read(sfd,buf,100)) < 0 ){
        printf("failed to read logout reply from server.\n\n");
		return -1;
	}
	buf[n] = '\0';

	if(strstr(buf,"successful")){
		logstatus = 0;
        printf("logout successful!\n\n");
		return 0;
	}
        
	printf("%s\n",buf);
	return -1;	
}


int pregister(int sfd){	

	char buf[100] = {0};
	char username[32],password[32];
	while(1){
		printf("username:");
		fgets(username,32,stdin); //包含'\n'
		if(strchr(username,' ')){
			printf("space is not permitted in username\n");
			continue;
		}
		if(!strcmp(username,"\n")){
			continue;
		}
		if(strlen(username) > 24){
			printf("username should be <= 24 characters\n");
			continue;
		}
		if(!strcmp(username,".")){
			printf("username should not be a '.' \n");
			continue;
		}
		break;
	}
	
	while(1){
		strcpy(password,getpass("password:")); //包含'\n'
		if(strchr(password,' ')){
			printf("space is not permitted in password.\n");
			continue;
		}
		if(!strcmp(password,"\n")){
			continue;
		}
		if(strlen(password) > 24){
			printf("password should be <= 24 characters.\n");
			continue;
		}
		break;
	}
	char tmppass[32] = {0};
	while(1){
		strcpy(tmppass,getpass("confirm password:")); //包含'\n'
		if(strchr(tmppass,' ')){
			printf("space is not permitted in password.\n");
			continue;
		}
		if(strlen(tmppass) > 24){
			printf("password should be <= 24 characters.\n");
			continue;
		}
		break;
	}
	
	if(strcmp(password,tmppass)){
        printf("password inputs differ,pleaes re_register.\n\n");
		dprintf(sfd,"register failed\n");
		return -1;
	}

	strtok(username,"\n");
	strtok(password,"\n");
	dprintf(sfd,"%s %s\n",username,password);

	int n = 0;
	if((n = read(sfd,buf,100)) < 0 ){
        printf("failed to read register reply from server.\n\n");
		return -1;
	}
	buf[n] = '\0';
    printf("%s\n",buf);
	
	return 0;
}

int pcheckon(int sfd){

    int cnt = 0;
	char sizebuf[64] = {0};

    ssize_t n = 0;
    if((n = read(sfd,sizebuf,64)) <= 0){
		perror("read error");
		printf("failed to get size of userlist.\n");
		return -1;
	}
        
    sizebuf[n] = '\0';
    printf("%s",sizebuf+10);	//消息自带\n
    sscanf(sizebuf,"server:@. chaters online: %d\n",&cnt);
	if(cnt == 0)
		return 0;	
		
    char msgbuf[1000] = {0};
    while(1){
        if((n = read(sfd,msgbuf,1000)) <= 0){
			perror("read error");
			printf("failed to get size of userlist.\n");
			break;
		}            
        msgbuf[n] = '\0';
	    printf("%s",msgbuf+10);
		if(strstr(msgbuf,"[over]"))  //消息自带\n\n
			break;
    }

	return cnt;    
}

int pcheckfiles(int sfd){

	ssize_t n = 0;
	char verify[256] = {0};	
	if((n = read(sfd,verify,256)) <= 0){
		perror("read error");
		printf("failed to get online filelist from server.\n\n");
		return -1;
	}
	verify[n] = '\0'; //无论文件夹是否存在 都不会输出第一个verify本身

	//共享文件夹不存在 则退出
	if(strstr(verify,"no shared files online.")){
		printf("no shared files online.\n\n");
		return -1;
	}

	//共享文件夹存在 无文件则退出 有文件则发文件
	char listbuf[1000] = {0};
	while(1){
		if((n = read(sfd,listbuf,256)) <= 0){
			perror("read error");
			printf("failed to get online filelist from server.\n");
			return -1;
		}
		listbuf[n] = '\0';
		printf("%s",listbuf+10);
		
		if(strstr(listbuf,"[over]") || strstr(listbuf,"no shared files online."))
			break;
	}

	return 0;
}

int ptalk(int sfd){

    char reply[128] = {0};
	int r = 0;
	if((r = read(sfd,reply,128)) < 0){
        printf("failed to get reply from server!\n\n");
		return -1;
	}
	reply[r] = '\0';
	if(!strstr(reply,"successful")){
        printf("%s\n",reply);
		return -1;
	}
	
	time_t t = time(NULL);
	struct tm *today = localtime(&t);
	char date[32] = {0};
	sprintf(date,"%02d%02d%02d",today->tm_year+1900,today->tm_mon+1,today->tm_mday);
	
    char logpath[256] = {0};
	char cwd[256] = {0};
	getcwd(cwd,256);
	sprintf(logpath,"%s%s%s%s",cwd,"/",myname,"_log/");

    if(access(logpath,R_OK|W_OK|X_OK) == -1){
        if(mkdir(logpath,0777) == -1){
            perror("mkdir error");
            printf("\n");
            return -1;
        }else
			printf("dir created OK:%s\n",logpath);
    }
	
	char logname[64] = {0};
	sprintf(logname,"%s%s%s%s",myname,"_chatlog_",date,".txt");
	strcat(logpath,logname);

	pfile = fopen(logpath,"a");
	if(pfile == NULL)
		printf("failed to open chatlog.\n");

	printf("\n");
	pthread_t tid1,tid2;
    if(pthread_create(&tid1,0,thread_send,(void*)&sfd) != 0){
		dprintf(sfd,"@. :quit\n");
        printf("error: failed to create thread_send.\n\n");
	    return -1;
	}
    if(pthread_create(&tid2,0,thread_recv,(void*)&sfd) != 0){
		dprintf(sfd,"@. :quit\n");
        printf("error : failed to create thread_recv.\n\n");
        return -1;
	}
	
	if(pthread_join(tid1,NULL) == 0 || pthread_join(tid2,NULL) == 0){
		pthread_cancel(tid1);
		pthread_cancel(tid2);
	}

	fclose(pfile);
	pfile = NULL;    
    printf("\n");
	return 0;
}

void* thread_send(void* psfd){	

	time_t t = 0;
	struct tm *today = NULL;
	int sfd = *(int*)psfd;
	int online = 0;
	char msg[1000] = {0};
	char tmpmsg[1000] = {0};
	char filepath[100] = {0};
	char toname[32] = {0};
	char tmptoname[32] = {0};
	char atme[32] = {0};
	sprintf(atme,"%s%s","@",myname);

	while(1){
		fgets(msg,1000,stdin); //包含\n\0

		if(strstr(msg,atme))
			continue;
		if(!strcmp(msg,"\n") || !strcmp(msg," \n"))//空白消息,只包含\n字符
			continue;
		
		//规定群发@.之后，所有消息都带有@<toname>
		if(msg[0] == '@'){ //如果指定接收人，则修改toname为给定值;
			//检查是否为文件上传下载请求
			if(strstr(msg,":upload $") || strstr(msg,":download $")){
				printf("@<toname> for :upload or :download is illegal.\n");
				continue;
			}
			
			//如果本次目标用户名同上次不一样 就重新设定在线状态
			sscanf(msg,"@%s",toname);
			if(strcmp(toname,tmptoname)){
				online = 0;
				strcpy(tmptoname,toname);
			}

			//验证文件命令合规性
			if(strstr(msg,":file ")){				
				if(strlen(toname) == 1 && toname[0] == '.'){
					printf("can not broadcast file by @.\n");
					continue;
				}				
				if(!strstr(msg,"$")){
					printf("$filepath should be designated.\n");
					continue;
				}
				sscanf(msg,"%*[^$]$%s",filepath);
				strtok(filepath,"\n");
				if(strlen(filepath) == 0){
					printf("filepath should not be null.\n");
					continue;
				}		

			//验证消息命令合规性		
			}else{
				if(sscanf(msg,"%*s %s\n",tmpmsg) == 0) 
					continue; 
			}

			//将合规的文件命令或消息命令发送到服务器
			dprintf(sfd,"%s",msg); //msg 包含@toname 和\n

			if(online == 0){
				//printf("checking @toname online status:\n");
				//服务器验证toname是否存在,存在返回[verify]: OK. 不存在返回[verify]: NOL.
				pthread_mutex_lock(&mutex1);
				pthread_cond_wait(&cond,&mutex1); 
				pthread_mutex_unlock(&mutex1);
				//任何目标用户不在线的消息或文件命令,都不会被转发 且不生成聊天记录
				if(ncond != 3) continue;
				online = 1;
				ncond = 0;
			}
			if(strstr(msg,":file "))
				if(pfile_send(sfd,filepath,toname) == -1) continue;			
			
		//不带@<toname>,补加@. 
		}else{
			if(strstr(msg,":file ")){ //不允许进行文件群发
				printf("@toname should be designated.\n");
				continue;
			}
			if(strstr(msg,":online\n")){
            	dprintf(sfd,"@. %s",msg);
            	continue;
       		}
			if(strstr(msg,":files\n")){ 
				dprintf(sfd,"@. %s",msg);
				continue;
			}
			if(strstr(msg,":help\n")){
				phelp();
				continue;
			}

			//@. :upload $<filepath> 上传文件至服务器
			//@. :download $<filepath> 从服务器下载文件
			if(strstr(msg,":upload ") || strstr(msg,":download ")){
				if(!strstr(msg,"$")){
					printf("$filepath should be designated.\n");
					continue;
				}
				
				sscanf(msg,"%*[^$]$%s",filepath);
				strtok(filepath,"\n");
				if(strlen(filepath) == 0){
					printf("filepath should not be null.\n");
					continue;
				}

				dprintf(sfd,"@. %s",msg); //msg 包含\n

				if(strstr(msg,":upload")){
					if(pfile_upload(sfd,filepath) == -1) continue;
				}
				else{ //在接收线程中启动下载模块，并根据下载结果决定是否生成聊天记录
					continue;
				}					
			}
			//群发消息
			else
				dprintf(sfd,"@. %s",msg); //msg 包含\n			
		}
		
		t = time(NULL);
		today = localtime(&t);
		pthread_mutex_lock(&mutex);
        fprintf(pfile,"%02d:%02d:%02d me: %s",today->tm_hour,today->tm_min,today->tm_sec,msg);
		pthread_mutex_unlock(&mutex);
        //聊天界面输入:exit回车，退出聊天界面
		if(!strcmp(msg,":exit\n"))
			return (void*)0;
	}
}

void* thread_recv(void* psfd){	

	time_t t = 0;
	struct tm *today = NULL;
	int sfd = *(int*)psfd;
	char msgbuf[1000] = {0};
	char realmsg[1000] = {0};
    char filepath[256] = {0};
	char fromname[32] = {0};
	char toname[32] = {0};
	int lenfrom = 0;
	int lento = 0;
    ssize_t n = 0;

	while(1){
		if((n = read(sfd,msgbuf,1000)) <= 0){ //若服务器退出，则退出
			perror("read");
			return (void*)-1;
		}		
		msgbuf[n] = '\0';

		//所有的消息格式都是msgbuf = fromname:@toname realmsg
        sscanf(msgbuf,"%[^:]",fromname);
		lenfrom = strlen(fromname);
		sscanf(msgbuf,"%*[^@]@%s",toname);
		lento = strlen(toname);
		strcpy(realmsg,msgbuf+lenfrom+lento+3);
   //   printf("fromname=%s toname=%s realmsg=%s",fromname,toname,realmsg);

		//:file 命令默认不能群发 所以不会在这一步执行
        //大多数的消息都不会含有认证信息[verify]:
        if(!strstr(realmsg,"[verify]:")){   //不显示,不打印[verify]:消息
            //群发消息
            if(strlen(toname) == 1 && toname[0] == '.'){
                if(!strcmp(fromname,"server")){
					if(strstr(realmsg,":download")){
						sscanf(realmsg,"%*[^$]$%s",filepath);
						strtok(filepath,"\n");
						if(pfile_download(sfd,filepath) == -1) continue;
					}else{
						printf("%s",realmsg);
						continue;
					}
                }else
                    printf("%s: %s",fromname,realmsg);
            //单发消息
            }else
				printf("%s:@%s %s",fromname,toname,realmsg);

            //此时realmsg不包含fromname:@toname 也不包含[verify]:类认证消息
            if(strstr(realmsg,":file") && strstr(realmsg,"$")){
                sscanf(realmsg,"%*[^$]$%s",filepath);
				strtok(filepath,"\n");
                if(pfile_recv(sfd,filepath,fromname,toname) == -1){
                    continue;   //文件接收失败的话，接收请求就不写入日志
                }
                memset(filepath,0,256);
            }

            t = time(NULL);
            today = localtime(&t);
            pthread_mutex_lock(&mutex);
            fprintf(pfile,"%02d:%02d:%02d %s: %s",today->tm_hour,today->tm_min,today->tm_sec,fromname,realmsg);
            pthread_mutex_unlock(&mutex);
        }
        //若对方确认接受文件,则设置ncond值
        else{
			// printf("%s\n",realmsg);
            if(!strcmp(realmsg,"[verify]: OK.\n")){
                ncond = 1;
                pthread_cond_signal(&cond);
            }
            else if(!strcmp(realmsg,"[verify]: NO.\n")){
                ncond = 0;
                pthread_cond_signal(&cond);
            }
            else if(!strcmp(realmsg,"[verify]: CC.\n")){
                ncond = 2;
                pthread_cond_signal(&cond);
            }
            else if(!strcmp(realmsg,"[verify]: SS.\n")){
                ncond = -1;
                pthread_cond_signal(&cond);
            }
			else if(!strcmp(realmsg,"[verify]: OL.\n")){
                ncond = 3;
                pthread_cond_signal(&cond);
            }
            else if(!strcmp(realmsg,"[verify]: NOL.\n")){
				printf("@toname is not online!\n");
                ncond = -1;
                pthread_cond_signal(&cond);
            }
            else if(!strcmp(realmsg,"[verify]: QUIT.\n")){
                printf("\n[msg]: A relogin action pushed you offline.\n"
                       "You may try relogin,or change password by command:chpass\n\n"
			           );
				//即使客户端发出消息让服务器退出，服务器也来不及接收消息
				//所以客户端强制退出以后，留下的客户线程，必须服务器自行清理
				exit(-1);
			}
			//printf("%s",realmsg);
        }
        memset(msgbuf,0,1000);
        memset(realmsg,0,1000);
	}

	return (void*)-1;
}

char* ppath_parse(char* filepath,char* path){
	//解析文件名
	char childpath[256] = {0};
	char* filename = NULL;
	char cwd[100] = {0};
	getcwd(cwd,100);

	//解析目标路径
	if(strstr(filepath,"/")){
		filename = 1 + strrchr(filepath,'/');
		strcpy(childpath,filepath);
		//去掉最后一个/及其后面的内容
		strrchr(childpath,'/')[0] = '\0';
	}else{
		filename = filepath;
		strcpy(childpath,cwd);
	}
	//printf("path=%s name=%s\n",childpath,filename);

	//解析真实路径 path为外部传入的空文件名
	//1 ~ home目录起头
	if(childpath[0] == '~'){
		//1.1 有子目录
		if(strlen(childpath) >1){
			sprintf(path,"%s%s",getenv("HOME"),strtok(childpath,"~"));
			//strtok()一般情况下,将出现的字符全部设置为\0，
			//然后返回剩下的字符串中不为\0的首地址
		}else
			//1.2 没有子目录
			strcpy(path,getenv("HOME"));
	//2 / 根目录起头
	}else if(childpath[0] == '/')
		strcpy(path,childpath);
	//3 .. 上层目录起头 
	else if(strlen(childpath) > 1 && childpath[0] == '.' && childpath[1] == '.'){
		strcpy(path,cwd); //拷贝当前目录
		//将倒数第一个/ 设置为\0 ，所得即是上层目录
		strrchr(path,'/')[0] = '\0';
		//3.1 有子目录
		if(strlen(childpath) > 2)
			//直接跳过.. 将后面的子目录连缀至上层路径path
			strcat(path,childpath+2);
		//3.2 没有子目录
			//什么都不干
	//4 . 当前目录起头
	}else if(childpath[0] == '.'){
		//path保存当前路径
		strcpy(path,cwd);
		//4.1 有子目录
		if(strlen(childpath) > 1)
			//跳过. 并连缀到当前目录
			strcat(path,childpath+1);
		//4.2 没有子目录
			//啥都不干
	//5 其他任意字符起头 通常表示当前目录下的子目录
	}else{
		strcpy(path,cwd);
		strcat(path,"/");
		strcat(path,childpath);
	}

	//路径解析完成，得到包含文件名的绝对路径	
	strcat(strcat(path,"/"),filename);
	return path;
}

int pfile_send(int sfd,char* filepath,char* toname){
	printf("\nfile_send: %s\n",filepath);

	//toname最长25个字节
	int len = strlen(toname);
	//解析相对路径，得到绝对路径
	char path[256] = {0};
	ppath_parse(filepath,path);		
	
	//获取并发送文件大小
	int size = 0;
	struct stat filestat = {0};
	if(stat(path,&filestat) == -1){
		dprintf(sfd,"@%s $staterr$\n",toname);
		perror("stat error");
		printf("\n");
		return -1;
	}
	size = filestat.st_size;
	if(size == 0){
		dprintf(sfd,"@%s $sizeerr$\n",toname);
		printf("filesize=0,failed to send file.\n\n");
		return -1;
	}
	//size > 0，则发送文件大小	
	dprintf(sfd,"@%s filesize=%d\n",toname,size);

	//注意,由于消息接收线程的持续存在,消息发送线程实际是收不到认证消息的
	//所以需要通过cond条件变量,实现收发线程间的同步
	pthread_mutex_lock(&mutex1);
	pthread_cond_wait(&cond,&mutex1);
	pthread_mutex_unlock(&mutex1);
	if(ncond != 1){
		printf("error: recver failed to recv file.\n");
		return -1;
	}
	
	//打开本地文件，并根据打开结果向对方发送进展情况
	FILE* psendfile = fopen(path,"r");
	if(psendfile == NULL){
		dprintf(sfd,"@%s $openerr$\n",toname);
		perror("fopen error");
		fclose(psendfile);
		printf("\n");
		return -1;
	}
	dprintf(sfd,"@%s openfile OK.\n",toname);
	
	int n = 0,w = 0;
	int wsum = 0;
	char filebuf[900] = {0}; //不超过服务器接收范围
	while(1){
		//经过信号量通知,才能开始发送
		pthread_mutex_lock(&mutex1);
		pthread_cond_wait(&cond,&mutex1); 
		pthread_mutex_unlock(&mutex1);
		if(ncond != 2){
			printf("error: got no [verify] CC. sending stopped.\n\n");
			fclose(psendfile);
			psendfile = NULL;
			return -1;
		}

		if((n = fread(filebuf,1,900,psendfile)) < 0){
			dprintf(sfd,"@%s $readerr$\n",toname);
			ferror(psendfile);
			printf("\n");
			fclose(psendfile);
			psendfile = NULL;
			return -1;
		}
		filebuf[n] = '\0';

		//如果不指定toname,则toname = ".";
		w = dprintf(sfd,"@%s %s\n",toname,filebuf); //增加\n以出尽缓存

		wsum += w-len-3;
		printf("sent: %d bytes, %%%.2lf...\n",w-len-3,wsum*100.0/size);
		ncond = 1;		
		if(wsum >= size)  break;
	}
	
	ncond = 0; //发送完毕之后重置判断条件
	fclose(psendfile);
	psendfile = NULL;
	printf("file size=%d sent successful.\n\n",size);

	return 0;
}

int pfile_recv(int sfd,char* filepath,char* fromname,char* toname){
	printf("\nfile_recv: %s\n",filepath);

	char* filename = NULL;
	if(strstr(filepath,"/"))
		filename = 1 + strrchr(filepath,'/');
	else
		filename = filepath;
	//printf("name=%s\n",filename);

	//因为不允许进行文件群发,所以所有的文件转发都是定向单发

	//接收文件大小
	int size = 0;
	char sizebuf[64] = {0};
	int n = 0;
	if((n = read(sfd,sizebuf,64)) < 0){
		perror("read error");
		printf("\n");
		return -1;
	}
	sizebuf[n] = '\0';
	if(strstr(sizebuf,"$staterr$") || strstr(sizebuf,"$sizeerr$")){
		printf("sender failed to fetch file size.\n\n");
		return -1;
	}
	//size > 0，才会接收到文件大小
	sscanf(sizebuf,"%*s filesize=%d\n",&size);
//	printf("size=%d\n",size);

	//创建接收文件的文件夹
    char cwd[100] = {0};
    char recvpath[256] = {0};
    getcwd(cwd,100);
	sprintf(recvpath,"%s%s%s%s",cwd,"/",myname,"_file/");

    if(access(recvpath,R_OK|W_OK|X_OK) == -1){
        if(mkdir(recvpath,0777) == -1){
            dprintf(sfd,"@%s [verify]: NO.\n",fromname);
            perror("mkdir error");
            return -1;
        }else
			printf("dir created OK:%s\n",recvpath);
    }
	
    strcat(recvpath,filename);

	//注意:此处文件接收方向发送方发送验证时,如果此前没有验证过fromname是否在线,
	//则服务器也会对fromname进行在线验证,就会受到server:@. [verify]: OL.\n的消息
	//但由于文件接收函数阻塞了thread_recv(),所以接收线程接收不到验证消息
	//这样验证消息就进入到了pfile_recv()当中,就成为了噪音!必须对此验证消息进行排除.
	//由于发送方必然在线,所以收到的消息一定是server:@. [verify]: OL.\n
    dprintf(sfd,"@%s [verify]: OK.\n",fromname);

	//验证发送方文件是否打开成功 $openerr$ 或 fileopen OK.
	if((n = read(sfd,sizebuf,64)) < 0){
		perror("read error");
		printf("\n");
		return -1;
	}
	sizebuf[n] = '\0';
	if(strstr(sizebuf,"$openerr$")){
		printf("error: sender failed to open file.\n");
		return -1;
	}
	//如果收到的是服务器的在线验证消息,则继续接收发送方的文件打开消息
	if(!strcmp(sizebuf,"server:@. [verify]: OL.\n")){
		if((n = read(sfd,sizebuf,64)) < 0){
			perror("read error");
			printf("\n");
			return -1;
		}
		sizebuf[n] = '\0';
		if(strstr(sizebuf,"$openerr$")){
			printf("error: sender failed to open file.\n");
			return -1;
		}
	}

	//此时发送方已经进入文件发送循环 等候接收方发送 [verify]: CC.消息
	//确认就会开始发送 否则发送方退出文件发送循环
    FILE* precvfile = fopen(recvpath,"w");
	if(precvfile == NULL){
		dprintf(sfd,"@%s [verify]: SS.\n",fromname);
		perror("fopen error");
		fclose(precvfile);
		printf("\n");
		return -1;
	}
	
	int lenfrom = strlen(fromname);
	int lento = strlen(toname);
	int r = 0;
	int w = 0;
	int wsum = 0;
	char filebuf[1000] = {0};
	char realmsg[1000] = {0};
	int lenreal = 0;
	//因为read()返回次数不确定，所以循环次数不可以与发送次数一致
	while(1){
		//通知发送方可以发送了
		dprintf(sfd,"@%s [verify]: CC.\n",fromname);

		r = read(sfd,filebuf,1000); //首先进入等待状态,阻塞接收
		if(r < 0){ //格式为fromname:@toname realmsg\n
			dprintf(sfd,"@%s [verify]: SS.\n",fromname);
			perror("read error");
			fclose(precvfile);
			precvfile = NULL;
			printf("\n");
			return -1;
		}
		filebuf[r] = '\0';
		//一共接收r个有效字符,
		//格式为fromname:@toname realmsg\n
		strcpy(realmsg,filebuf+lenfrom+lento+3);
		//绝对不能用sscanf(),因为它遇空格或者换行就会停止

		if(strstr(realmsg,"$readerr$")){
			printf("sender failed to send file content.\n\n");
			fclose(precvfile);
			precvfile = NULL;
			return -1;
		}
		lenreal = strlen(realmsg); //包含\n
		realmsg[lenreal-1] = '\0'; // \n替换为\0

		if((w = fwrite(realmsg,1,strlen(realmsg),precvfile)) < 0){
			dprintf(sfd,"@%s [verify]: SS.\n",fromname);
			ferror(precvfile);
			fclose(precvfile);
			precvfile = NULL;
			printf("\n");
			return -1;
		}

		wsum += w;
		printf("recved: %d bytes, %%%.2lf...\n",w,wsum*100.0/size);
		if(wsum >= size) break;
	}

	fclose(precvfile);
	precvfile = NULL;
	printf("file size=%d recved successful.\n\n",size);

	return 0;
}

int pfile_upload(int sfd,char* filepath){
	printf("\nfile_upload: %s\n",filepath);

	//解析相对路径，得到绝对路径
	char path[256] = {0};
	ppath_parse(filepath,path);			
	
	//获取并发送文件大小
	int size = 0;
	struct stat filestat = {0};
	if(stat(path,&filestat) == -1){
		dprintf(sfd,"@. $staterr$\n");
		perror("stat error");
		printf("\n");
		return -1;
	}
	size = filestat.st_size;
	if(size == 0){
		dprintf(sfd,"@. $sizeerr$\n");
		printf("filesize=0,failed to send file.\n\n");
		return -1;
	}
	//size > 0，则发送文件大小	
	dprintf(sfd,"@. filesize=%d\n",size);

	//注意,由于消息接收线程的持续存在,消息发送线程实际是收不到认证消息的
	//所以需要通过cond条件变量,实现收发线程间的同步
	pthread_mutex_lock(&mutex1);
	pthread_cond_wait(&cond,&mutex1);
	pthread_mutex_unlock(&mutex1);
	if(ncond != 1){
		printf("error: recver failed to recv file.\n");
		return -1;
	}
	
	//打开本地文件，并根据结果向服务器发送进展情况
	FILE* psendfile = fopen(path,"r");
	if(psendfile == NULL){
		dprintf(sfd,"@. $openerr$\n");
		perror("fopen error");
		fclose(psendfile);
		printf("\n");
		return -1;
	}
	dprintf(sfd,"@. fileopen OK.\n");

	int n = 0,w = 0;
	int wsum = 0;
	char filebuf[900] = {0}; //不超过服务器接收范围
	while(1){
		pthread_mutex_lock(&mutex1);
		pthread_cond_wait(&cond,&mutex1); //经过信号量通知,才能开始发送
		pthread_mutex_unlock(&mutex1);
		if(ncond != 2){
			printf("error:file sending process failed.\n\n");
			fclose(psendfile);
			psendfile = NULL;
			return -1;
		}

		if((n = fread(filebuf,1,900,psendfile)) < 0){
			dprintf(sfd,"@. $readerr$\n");
			ferror(psendfile);
			printf("\n");
			fclose(psendfile);
			psendfile = NULL;
			return -1;
		}
		filebuf[n] = '\0';

		w = dprintf(sfd,"@. %s\n", filebuf); //增加\n以出尽缓存

		wsum += w-4;
		printf("sent: %d bytes, %%%.2lf...\n",w-4,wsum*100.0/size);
		ncond = 1;		
		if(wsum >= size)  break;
	}
	
	ncond = 0; //发送完毕之后重置判断条件
	fclose(psendfile);
	psendfile = NULL;
	printf("file size=%d sent successful.\n\n",size);

	return 0;
}

int pfile_download(int sfd,char* filepath){
	printf("\nfile_download: %s\n",filepath);

	//获取文件名
	char* filename = NULL;
	if(strstr(filepath,"/"))
		filename = 1 + strrchr(filepath,'/');
	else
		filename = filepath;
	printf("name=%s\n",filename);

	//接收文件大小
	int size = 0;
	char sizebuf[64] = {0};
	int n = 0;
	if((n = read(sfd,sizebuf,64)) < 0){
		perror("read error");
		printf("\n");
		return -1;
	}
	sizebuf[n] = '\0';
	if(strstr(sizebuf,"$staterr$") || strstr(sizebuf,"$sizeerr$")){
		printf("sender failed to fetch file size.\n\n");
		return -1;
	}
	//size > 0，才会接收到文件大小
	sscanf(sizebuf,"%*s filesize=%d\n",&size);
//	printf("size=%d\n",size);

	//创建接收文件的文件夹
    char cwd[100] = {0};
    char recvpath[256] = {0};
    getcwd(cwd,100);
	sprintf(recvpath,"%s%s%s%s",cwd,"/",myname,"_file/");

    if(access(recvpath,R_OK|W_OK|X_OK) == -1){
        if(mkdir(recvpath,0777) == -1){
            dprintf(sfd,"@. [verify]: NO.\n");
            perror("mkdir error");
            return -1;
        }else
			printf("dir created OK:%s\n",recvpath);
    }	
    strcat(recvpath,filename);
	dprintf(sfd,"@. [verify]: OK.\n");

	//验证发送方文件是否打开成功
	if((n = read(sfd,sizebuf,64)) < 0){
		perror("read error");
		printf("\n");
		return -1;
	}
	sizebuf[n] = '\0';
	if(strstr(sizebuf,"$openerr$")){
		printf("error: sender failed to open file.\n");
		return -1;
	}

    FILE* precvfile = fopen(recvpath,"w");
	if(precvfile == NULL){
		dprintf(sfd,"@. [verify]: SS.\n");
		perror("fopen error");
		fclose(precvfile);
		printf("\n");
		return -1;
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
		dprintf(sfd,"@. [verify]: CC.\n");

		r = read(sfd,filebuf,1000); //首先进入等待状态,阻塞接收
		if(r < 0){ //格式为@. realmsg\n
			dprintf(sfd,"@. [verify]: SS.\n");
			perror("read error");
			fclose(precvfile);
			precvfile = NULL;
			printf("\n");
			return -1;
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
			return -1;
		}
		lenreal = strlen(realmsg); //包含\n
		realmsg[lenreal-1] = '\0'; // \n替换为\0

		if((w = fwrite(realmsg,1,strlen(realmsg),precvfile)) < 0){
			dprintf(sfd,"@. [verify]: SS.\n");
			ferror(precvfile);
			fclose(precvfile);
			precvfile = NULL;
			printf("\n");
			return -1;
		}

		wsum += w;
		printf("recved: %d bytes, %%%.2lf...\n",w,wsum*100.0/size);
		if(wsum >= size) break;
	}

	fclose(precvfile);
	precvfile = NULL;
	printf("file size=%d recved successful.\n\n",size);

	
	return 0;
}

int pconnect(char* ip){

	SA4 serv;
	serv.sin_family = AF_INET;
	serv.sin_port = htons(8080);
	serv.sin_addr.s_addr = inet_addr(ip);
	
	int sfd = socket(AF_INET,SOCK_STREAM,0);
	if(sfd == -1){
		perror("socket");
		return -1;
	}

	int c = connect(sfd,(SA*)&serv,sizeof(serv));
	if(c == -1){
		perror("connect");
		return -1;
	}

	return sfd;
}


