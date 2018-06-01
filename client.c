 #include "client.h" //client.c

char cmd[32] = {0};
int logstatus = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
int ncond = 0;
FILE* pfile = NULL;
char myname[32] = {0};

int pcommand(void){

	while(1){
		printf("\ncommand:");
		fflush(stdin);
		fgets(cmd,20,stdin);//包含'\n'
		if(strchr(cmd,' ')){
			printf("space is not permitted in command.\n");
			continue;
		}
		if(strlen(cmd) == 0){
			printf("command can't be null.\n");
			continue;
		}
		break;
	}
	if(!strcmp(cmd,"help\n"))
		return HELP;
	else if(!strcmp(cmd,"login\n"))
		return LOGIN;
	else if(!strcmp(cmd,"logout\n"))
		return LOGOUT;
	else if(!strcmp(cmd,"register\n"))
		return REGISTER;
	else if(!strcmp(cmd,"online\n"))
		return CHECKON;
	else if(!strcmp(cmd,"talk\n"))
		return TALK;
	else if(!strcmp(cmd,"quit\n"))
		return QUIT;
	else 
		return UNKNOWN;
}

void phelp(void){
    printf("commands:\n"
           "\tlogin		set logstatus on;\n"
           "\tlogout		set logstatus off\n"
           "\tregister	register user ID;\n"
           "\tonline		check online list;\n"
           "\ttalk		enter talkroom;\n"
           "\tquit		quit this client.\n\n"
           
           "talking:\n"
           "\tuse format \"@<name> <msg>\" to designate a single recver,and msgs will be sent privately.\n"
           "\tif \"@<name>\" is not designated, msgs will be broatcasted to everyone by default.\n"
           "\tinput \":exit\" to exit chatroom, and \":online\" to check chaters online.\n\n"
           
           "file sharing (in chatroom):\n"
           "\tuse format \"@<name> :file $<filepath>\" to send a file to <name>(critical) privately.\n"
           "\tfiles should not be broadcasted, but you can share files by uploading them to server.\n"
           "\tinput \":upload $<filepath>\" to upload a file to sever,\":download $<filename>\" to download a file from server,\n"
           "\tand \":checkfiles\" to check files on sever.\n"
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

void plogout(void){
	if(logstatus == 1)
		logstatus = 0;
		
	printf("logstatus off!\n");
}

int plogin(int sfd){

	if(logstatus == 1){
		printf("can not relogin, please retry.\n");
		return -1;
	}
	
	if(psendcmd(sfd) == -1)
		return -1;

	char buf[100] = {0};
	char username[32],password[32];
	while(1){
		printf("username:");
		fgets(username,32,stdin);//包含'\n'
		if(strchr(username,' ')){
			printf("space is not permitted in username.\n");
			continue;
		}
		if(strlen(username) > 24){
			printf("username should be <= 24 characters\n");
			continue;
		}
		break;
	}
	
	while(1){
		strcpy(password,getpass("password:"));//包含'\n'
		if(strchr(password,' ')){
			printf("space is not permitted in password.\n");
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
		printf("failed to read login reply from server.\n");
		return -1;
	}
	buf[n] = '\0';

	if(strstr(buf,"successful")){
		logstatus = 1;
		printf("logstatus on!\n");
	}else
		printf("%s",buf);

	return 0;
}

int pregister(int sfd){
	if(logstatus == 1){
		printf("please logout first!.\n");
		return -1;
	}

	if(psendcmd(sfd) == -1)
		return -1;

	char buf[100] = {0};
	char username[32],password[32];
	while(1){
		printf("username:");
		fgets(username,32,stdin);//包含'\n'
		if(strchr(username,' ')){
			printf("space is not permitted in username\n");
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
		strcpy(password,getpass("password:"));//包含'\n'
		if(strchr(password,' ')){
			printf("space is not permitted in password.\n");
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
		strcpy(tmppass,getpass("confirm password:"));//包含'\n'
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
		printf("password inputs differ,pleaes re_register.\n");
		dprintf(sfd,"register failed\n");
		return -1;
	}

	strtok(username,"\n");
	strtok(password,"\n");
	dprintf(sfd,"%s %s\n",username,password);

	int n = 0;
	if((n = read(sfd,buf,100)) < 0 ){
		printf("failed to read register reply from server.\n");
		return -1;
	}
	buf[n] = '\0';
	printf("%s",buf);
	
	return 0;
}

int pcheckon(int sfd){

	if(logstatus == 0){
		printf("please login first!\n");
		return -1;
	}
	if(psendcmd(sfd) == -1)
		return -1;
	
	int cnt = 0;
	char buf[16] = {0};
	int n = 0;
	if((n = read(sfd,buf,16)) <= 0)
		printf("failed to get size of userlist.\n");
	buf[n] = '\0';
	sscanf(buf,"%d\n",&cnt);
	printf("members online: %d\n",cnt);
	if(cnt == 0) return 0;
	
	char* userlist = (char*)malloc(32*cnt+100);//彻底杜绝内存不够?32不是已经够了吗
	if(userlist == NULL){
		printf("mem error:failed to malloc mem for userlist.\n");
		return -1;
	}
	userlist[0] = '\0';
	//printf("malloc done. starting reading userlist.\n");
	//为什么服务器发送成功了,但是read()函数经常不返回?
	if((n = read(sfd,userlist,32*cnt+100)) <= 0){
		printf("failed to get userlist from server.\n");
		return -1;
	}
	userlist[n] = '\0';//如果内存不足的话,有可能设置字符串结尾\0失败
	//字符串\0结尾设置不成功的话,就会无法正常输出
	printf("%s\n",strtok(userlist,"\n"));//接收到的userlist自带\n
	free(userlist);
	userlist = NULL;
		
	return cnt;
}

int ptalk(int sfd){

	if(logstatus == 0){
		printf("please login first!\n");
		return -1;
	}
	if(psendcmd(sfd) == -1)
		return -1;

	char reply[128] = {0};
	int r = 0;
	if((r = read(sfd,reply,128)) < 0){
		printf("failed to get reply from server!\n");
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
	
	char logname[256] = {0};
	strcpy(logname,myname);
	strcat(logname,"_chatlog_");
	strcat(logname,date);
	strcat(logname,".txt");

	pfile = fopen(logname,"a");
	if(pfile == NULL)
		printf("failed to open chatlog.\n");

	printf("\n");
	pthread_t tid1,tid2;
    if(pthread_create(&tid1,0,thread_send,(void*)&sfd) != 0){
		dprintf(sfd,":exit\n");
	    printf("error: failed to create thread_send.\n");
	    return -1;
	}
    if(pthread_create(&tid2,0,thread_recv,(void*)&sfd) != 0){
		dprintf(sfd,":exit\n");
        printf("error : failed to create thread_recv.\n");
        return -1;
	}
	
	if(pthread_join(tid1,NULL) == 0 || pthread_join(tid2,NULL) == 0){
		pthread_cancel(tid1);
		pthread_cancel(tid2);
	}

	fclose(pfile);
	pfile = NULL;
	return 0;
}

void* thread_send(void* psfd){
	
	time_t t = 0;
	struct tm *today = NULL;
	int sfd = *(int*)psfd;
	char msg[1000] = {0};
	char filepath[100] = {0};
	char toname[32] = {0};
	char atme[32] = {0};
	strcat(atme,"@");
	strcat(atme,myname);

	while(1){
		fgets(msg,1000,stdin);//包含\n\0

		if(strstr(msg,atme))
			continue;
		if(!strcmp(msg,"\n"))//空白消息,只包含\n字符
			continue;
        if(!strcmp(msg,":online\n")){
            dprintf(sfd,"%s",msg);
            pcheckon(sfd);
            continue;
        }
		
		//规定群发@.之后，所有消息都带有@
		if(msg[0] == '@'){//如果指定接收人，则修改toname为给定值;
			
			if(strstr(msg,":file")){
				sscanf(msg,"@%s",toname);
				if(strlen(toname) == 1 && toname[0] == '.'){
					printf("can not broadcast file by @.\n");
					continue;
				}
				
				if(!strstr(msg,"$")){
					printf("$filepath should be designated.\n");
					continue;
				}
				sscanf(msg,"%*[^$]$%s",filepath);
				
				dprintf(sfd,"%s",msg);//msg 包含@toname 和\n
				//首先判断toname是否存在,如果不存在,则返回
				pthread_mutex_lock(&mutex1);
				pthread_cond_wait(&cond,&mutex1);//经过通知,才能开始发送
				pthread_mutex_unlock(&mutex1);
				if(ncond != 1) continue;
				if(pfile_send(sfd,filepath,toname) == -1) continue;
				ncond = 0;
			}else
				dprintf(sfd,"%s",msg);//msg 包含@toname 和\n
		}else{//群发,补加@.
			if(strstr(msg,":file")){//不允许进行文件群发
				printf("@toname should be designated.\n");
				continue;
			}
			dprintf(sfd,"@. %s",msg);//msg 包含\n
		}
		
		t = time(NULL);
		today = localtime(&t);
		pthread_mutex_lock(&mutex);
        fprintf(pfile,"%02d:%02d:%02d me:%s\n",today->tm_hour,today->tm_min,today->tm_sec,msg);
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
	char filepath[100] = {0};
	char fromname[32] = {0};
	char toname[32] = {0};
	int lenfrom = 0;
	int lento = 0;

	int n = 0;	
	while(1){//服务器转发不再对字符串进行任何处理,如果原来包含\n,那么现在仍然有\n
		if((n = read(sfd,msgbuf,1000)) <= 0){//若服务器退出，则退出
			perror("read");
			return (void*)-1;
		}		
		msgbuf[n] = '\0';
		//所有的消息格式都是msgbuf = fromname:@toname realmsg
		sscanf(msgbuf,"%[^:]",fromname);//:之前的所有字符
		lenfrom = strlen(fromname);
		sscanf(msgbuf,"%*[^@]@%s",toname);
		lento = strlen(toname);
		strcpy(realmsg,msgbuf+lenfrom+lento+3);
	//printf("fromname=%s toname=%s realmsg=%s",fromname,toname,realmsg);

		//若对方确认接受文件,则设置ncond值
		if(!strcmp(realmsg,"[verify]: OK.\n")){
			ncond = 1;
			pthread_cond_signal(&cond);
		}
		if(!strcmp(realmsg,"[verify]: NO.\n")){
			ncond = 0;
			pthread_cond_signal(&cond);
		}
		if(!strcmp(realmsg,"[verify]: CC.\n")){
			ncond = 2;
			pthread_cond_signal(&cond);
		}
		if(!strcmp(realmsg,"[verify]: SS.\n")){
			ncond = -1;
			pthread_cond_signal(&cond);
		}
		if(!strcmp(realmsg,"@toname not online!\n")){
			ncond = -1;
			pthread_cond_signal(&cond);
		}
		
        if(!strstr(realmsg,"[verify]:")){//不显示,不打印[verify]:消息
			//群发则不含@toname，realmsg包含\n
			if(strlen(toname) == 1 && toname[0] == '.')
				printf("%s:%s",fromname,realmsg);
			else
				printf("%s:@%s %s",fromname,toname,realmsg);

            //此时msg不包含fromname:@toname 也不包含[verify]:类认证消息
            if(strstr(realmsg,":file") && strstr(realmsg,"$")){
                sscanf(realmsg,"%*[^$]$%s",filepath);
                if(pfile_recv(sfd,filepath,fromname,toname) == -1){
                    continue;//文件接收失败的话，接收请求就不写入日志
                }
            }

            t = time(NULL);
            today = localtime(&t);
            pthread_mutex_lock(&mutex);
            fprintf(pfile,"%02d:%02d:%02d %s\n",today->tm_hour,today->tm_min,today->tm_sec,msgbuf);
            pthread_mutex_unlock(&mutex);
        }
	}

	return (void*)-1;
}

int pfile_send(int sfd,char* filepath,char* toname){
	printf("pfile_send: start sending..\n");

	//toname最长25个字节
	//解析文件名
	char path[100] = {0};
	char childpath[100] = {0};
	char* name = NULL;
	char cwd[100] = {0};
	char tmpcwd[100] = {0};
	getcwd(cwd,100);
	getcwd(tmpcwd,100);
	int len = strlen(toname);
	
	//解析目标路径
	if(strstr(filepath,"/")){
		name = 1 + strrchr(filepath,'/');
		strcpy(childpath,filepath);
		//将childpath倒数第一个/ 设置为\0
		strrchr(childpath,'/')[0] = '\0';
	}else{
		name = filepath;
		strcpy(childpath,cwd);
	}
	printf("path=%s name=%s\n",childpath,name);

	//解析真实路径
	//1 ~ home目录起头
	if(childpath[0] == '~'){
		//1.1 有子目录
		if(strlen(childpath) >1){
			strcpy(path,getenv("HOME"));
			strcat(path,strtok(childpath,"~"));
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
		strcpy(path,cwd);//拷贝当前目录
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
	//路径解析完成
	chdir(path);
//	printf("working directory changed as:%s\n",path);

	//获取文件大小
	int size = 0;
	struct stat filestat = {0};
	if(stat(name,&filestat) == -1){
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
	//$file$在服务器转发过程中有特殊意义，
	//表示以原字符串风格转发,不添加来源姓名
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
	
	FILE* psendfile = fopen(name,"r");
	if(psendfile == NULL){
		dprintf(sfd,"@%s $openerr$\n",toname);
		perror("fopen error");
		printf("\n");
		return -1;
	}
	
	int n = 0,w = 0;
	int wsum = 0;
char filebuf[900] = {0};//不超过服务器接收范围
	while(1){
		pthread_mutex_lock(&mutex1);
		pthread_cond_wait(&cond,&mutex1);//经过信号量通知,才能开始发送
		pthread_mutex_unlock(&mutex1);
		if(ncond != 2){
			printf("error:file sending process failed.\n");
			return -1;
		}

		if((n = fread(filebuf,1,900,psendfile)) < 0){
			ferror(psendfile);
			return -1;
		}
		filebuf[n] = '\0';

		//如果不指定toname,则toname = ".";
		w = dprintf(sfd,"@%s %s\n",toname,filebuf);//增加\n以出尽缓存

		wsum += w-len-3;
		printf("sent: %d bytes, %%%.2lf...\n",w-len-3,wsum*100.0/size);
		ncond = 1;		
		if(wsum >= size)  break;
	}
	
	ncond = 0;//发送完毕之后重置判断条件
	fclose(psendfile);
	psendfile = NULL;
	chdir(cwd);
	printf("file size=%d sent successful.\n\n",size);

	return 0;
}

int pfile_recv(int sfd,char* filepath,char* fromname,char* toname){
	char* name = NULL;
	if(strstr(filepath,"/"))
		name = 1 + strrchr(filepath,'/');
	else
		name = filepath;
	printf("name=%s\n",name);

	//因为不允许进行文件群发,所以所有的文件转发都是定向单发

	//获取文件大小
	int size = 0;
	char sizebuf[64] = {0};
	int n = 0;
	if((n = read(sfd,sizebuf,32)) < 0){
		perror("read error");
		printf("\n");
		return -1;
	}
	sizebuf[n] = '\0';
	if(strstr(sizebuf,"$staterr$") || strstr(sizebuf,"$sizeerr$")){
		printf("sender failed to fetch file size.\n\n");
		return -1;
	}
	sscanf(sizebuf,"%*s filesize=%d\n",&size);
//	printf("size=%d\n",size);

	//根据文件大小,选择发送不同的认证消息
	
	if(size == 0){
		dprintf(sfd,"@%s [verify]: NO.\n",fromname);
		printf("filesize=0,failed to create file.\n\n");
		return -1;
	}	
	

    char cwd[100] = {0};
    char recvpath[256] = {0};
    getcwd(cwd,100);
    strcat(recvpath,cwd);
    strcat(recvpath,"/");
    strcat(recvpath,myname);
    strcat(recvpath,"/");

    if(access(recvpath,R_OK|W_OK|X_OK) == -1){
        if(mkdir(recvpath,0777) == -1){
            dprintf(sfd,"@%s [verify]: NO.\n",fromname);
            perror("mkdir error");
            return -1;
        }else
			printf("dir created OK:%s\n",recvpath);
    }
	
	chdir(recvpath);
    dprintf(sfd,"@%s [verify]: OK.\n",fromname);

    FILE* precvfile = fopen(name,"w");
	if(precvfile == NULL){
		dprintf(sfd,"@%s [verify]: SS.\n",fromname);
		perror("fopen error");
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
		r = read(sfd,filebuf,1000);//首先进入等待状态,阻塞接收
		if(r < 0){//格式为fromname:@toname realmsg\n
			dprintf(sfd,"@%s [verify]: SS.\n",fromname);
			perror("read error");
			printf("\n");
			ferror(precvfile);
			return -1;
		}
		filebuf[r] = '\0';
		//一共接收r个有效字符,
		//格式为fromname:@toname realmsg\n
		strcpy(realmsg,filebuf+lenfrom+lento+3);
		//绝对不能用sscanf(),因为它遇空格或者换行就会停止

		if(strstr(realmsg,"$openerr$") || strstr(realmsg,"$readerr$")){
			dprintf(sfd,"@%s [verify]: SS.\n",fromname);
			printf("sender failed to send file content.\n\n");
			return -1;
		}
		lenreal = strlen(realmsg);//包含\n
		realmsg[lenreal-1] = '\0';// \n替换为\0

		if((w = fwrite(realmsg,1,strlen(realmsg),precvfile)) < 0){
			dprintf(sfd,"@%s [verify]: SS.\n",fromname);
			ferror(precvfile);
			return -1;
		}

		wsum += w;
		printf("recved: %d bytes, %%%.2lf...\n",w,wsum*100.0/size);
		if(wsum >= size) break;
	}

	fclose(precvfile);
	precvfile = NULL;
	chdir(cwd);
	printf("file size=%d recved successful.\n\n",size);

	return 0;
}

int pquit(int sfd){
	
	psendcmd(sfd);
	exit(0);
}

int punknown(void){

	printf("command is not known,please reinput.\n");
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
