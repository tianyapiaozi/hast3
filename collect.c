#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

#include "hast3.h"
#include "communicate.h"

pid_t collect_pid = 0;

static int collect_main_loop(Env *env){
	int fd, i, sndcnt = 0, retry = 0, s;
	int message_len;
	short status;
	struct sockaddr_in addr;
	struct timespec timeout;
	Hast3_message * message;

	message_len = sizeof(Hast3_message) + (unsigned int)env->service_num * 
		sizeof(Hast3_message_entry);
	message = (Hast3_message *)malloc(message_len);
	if(message == NULL){
		fprintf(stderr, "Malloc error\n");
		exit(EXIT_FAILURE);
	}

	/* create what looks like an ordinary UDP socket */
	if ((fd=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		fprintf(stderr, "create socket error\n");
		exit(EXIT_FAILURE);
	}

	/* set up destination address */
	memset(&addr,0,sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=inet_addr(HEARTBEAT_GROUP);
	addr.sin_port=htons((uint16_t)env->port);

	/* connect to the partner */
	if(connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0){
		fprintf(stderr, "connect error\n");
		exit(EXIT_FAILURE);
	}

	/* fill the header part of message */
	strcpy(message->nodename, env->nodename);
	message->type = HAST3_MSG_BCAST;
	message->field_num = (short)env->service_num;

	/* set the timeout struct */
	timeout.tv_sec = (int)env->ha_interval;
	timeout.tv_nsec = (env->ha_interval-(int)env->ha_interval)*1000000000;

	/* loop forever */
	for(;;){

		/* get the status of each service */
		for(i = 0; i < env->service_num; i++){
			strcpy(message->data[i].service_name, env->services[i].name);
			if(system(env->services[i].statecmd) == 0)
				status = Service_Running;
			else{
				if(env->services[i].tried_cnt > env->max_try_no)
					status = Service_Failed;
				else
					status = Service_Nonrunning;
			}
			message->data[i].cmd_or_status = status;
		}

		/* fill the check sum part */
		message->checksum = 0;
		message->checksum = checksum((u_short *)message, message_len);

		/* check the return value of send */
		sndcnt = 0;
		retry = 0;
		while(retry < RETRYCNT && sndcnt < message_len){
			s = send(fd,message,message_len ,0);
			if(s < 0)
				retry++;
			else
				sndcnt += s;
		}
		nanosleep(&timeout, NULL);
	}

	/* should never go here */
	close(fd);
	exit(EXIT_FAILURE);
}

int start_collect(Env *env){
	int in, out, i;

	if(collect_pid != 0 && kill(collect_pid, 0) != -1)
		kill(collect_pid, SIGKILL);

	collect_pid = fork();
	if(collect_pid == -1){
		fprintf(stderr, "fork error");
		exit(EXIT_FAILURE);
	}
	else if(collect_pid == 0){
		in = open("/dev/null", O_RDONLY);
		out = open("/dev/null", O_WRONLY);

		dup2(in, 0);
		dup2(out, 1);
		dup2(out, 2);

		for(i = 3; i < NOFILE; i++)
			close(i);

		collect_main_loop(env);
		return STATUS_CLECT_ERR;
	}
	else{
		return STATUS_OK;
	}

}

int stop_collect(){
	kill(collect_pid, SIGKILL);
	return STATUS_OK;
}
