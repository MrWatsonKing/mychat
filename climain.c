#include "client.h"/////////////////////// climain.c
char cmd[32] = {0};
int logstatus = 0;

int main(int argc,char** argv){
#if 1	
	if(argc != 2){
		printf("Usage: clnt <ip>\n");
		return -1;
	}
	int sfd = pconnect(argv[1]);
#else
	int sfd = pconnect("127.0.0.1");
#endif

	if(sfd == -1){
		printf("pconnect fails\n");
		return -1;
	}	
    printf("successfully connected.\ncommand:help to view help list.\n\n");

	while(1){

        //get commands:
        while(1){
            printf("command:");
            fflush(stdin);
            fgets(cmd,20,stdin);//包含'\n'
            if(strchr(cmd,' ')){
                printf("space is not permitted in command.\n");
                continue;
            }
            if(!strcmp(cmd,"\n")){
                continue;
            }
            break;
        }
        // printf("%s",cmd);

        //execute commands.
        if(!strcmp(cmd,"help\n")){
            phelp();
        }
        else if(!strcmp(cmd,"login\n")){
            if(logstatus == 1){
                printf("can not relogin, please retry.\n\n");
                continue;
            }
            if(psendcmd(sfd) == -1){
                printf("\n");
                continue;
            }
            plogin(sfd);
        }
        else if(!strcmp(cmd,"shares\n")){
            if(logstatus == 0){
                printf("please login first!\n\n");
                continue;
            }
            if(psendcmd(sfd) == -1){
                printf("\n");
                continue;
            }
            pcheckfiles(sfd);
        }
        else if(!strcmp(cmd,"logout\n")){
            if(logstatus == 0){
                printf("not login yet!\n\n");
                continue;
            }
            if(psendcmd(sfd) == -1){
                printf("\n");
                continue;
            }
            plogout(sfd);
        }
        else if(!strcmp(cmd,"register\n")){
            if(logstatus == 1){
                printf("please logout first!.\n\n");
                continue;
            }
            if(psendcmd(sfd) == -1){
                printf("\n");
                continue;
            }
            pregister(sfd);
        }
        else if(!strcmp(cmd,"online\n")){
            if(logstatus == 0){
                printf("please login first!\n\n");
                continue;
            }
            if(psendcmd(sfd) == -1){
                printf("\n");
                continue;
            }
            pcheckon(sfd);
        }
        else if(!strcmp(cmd,"talk\n")){
            if(logstatus == 0){
                printf("please login first!\n\n");
                continue;
            }
            if(psendcmd(sfd) == -1){
                printf("\n");
                continue;
            }
            ptalk(sfd);
        }
        else if(!strcmp(cmd,"quit\n")){
            if(psendcmd(sfd) == -1){
                printf("\n");
                continue;
            }
            close(sfd);            
            exit(0);
        }else{
            printf("command is not known,please reinput.\n\n");
        }
	}
	
	return 0;
}

