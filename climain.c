#include "client.h"/////////////////////// climain.c
char cmd[32] = {0};
int logstatus = 0;

int main(int argc,char** argv){
/*	
	if(argc != 2){
		printf("Usage: clnt <ip>\n");
		return -1;
	}
	int sfd = pconnect(argv[1]);
*/
	int sfd = pconnect("127.0.0.1");
	if(sfd == -1){
		printf("pconnect fails\n");
		return -1;
	}	
	printf("successfully connected.\ncommand:help to view help list.\n");	

	while(1){

        //get commands:
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

        //execute commands.
        if(!strcmp(cmd,"help\n")){
            phelp();
        }
        else if(!strcmp(cmd,"login\n")){
            if(logstatus == 1){
                printf("can not relogin, please retry.\n");
                continue;
            }
            if(psendcmd(sfd) == -1)
                continue;
            plogin(sfd);
        }
        else if(!strcmp(cmd,"logout\n")){
            plogout();
        }
        else if(!strcmp(cmd,"register\n")){
            if(logstatus == 1){
                printf("please logout first!.\n");
                continue;
            }
            if(psendcmd(sfd) == -1)
                continue;
            pregister(sfd);
        }
        else if(!strcmp(cmd,"online\n")){
            if(logstatus == 0){
                printf("please login first!\n");
                continue;
            }
            if(psendcmd(sfd) == -1)
                continue;
            pcheckon(sfd);
        }
        else if(!strcmp(cmd,"talk\n")){
            if(logstatus == 0){
                printf("please login first!\n");
                continue;
            }
            if(psendcmd(sfd) == -1)
                continue;
            ptalk(sfd);
        }
        else if(!strcmp(cmd,"quit\n")){
            if(psendcmd(sfd) == -1)
                continue;
            pquit(sfd);
        }else{
            punknown();
        }
	}
	
	return 0;
}

