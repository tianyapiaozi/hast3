/* 
 * Copyright (C) 
 * 2011 - Jiliang Li(tjulijiliang@gmail.com)
 * This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */

/**
 * @file collect.c
 * @brief functions related to collect process.
 * @author Li Jiliang<tjulijiliang@gmail.com
 * @version 1.0
 * @date 2011-11-09
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/wait.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>

#include "hast3.h"
#include "communicate.h"
#include "collect.h"

static int get_service_status(Env *env,int service_index);
static int collect_system(const char* cmd);

extern sem_t mutex;

pid_t collect_pid = 0;

/**
 * @brief The main loop of the collect process, which basically collects the status information of all the services and multicasts it to the other nodes
 *
 * @param env Env struct
 *
 * @return The function should loop forever and return denotes an error.
 */
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
			if(get_service_status(env, i) == 0)
				status = Service_Running;
			else{
				sem_wait(&mutex);
				if(env->services[i].tried_cnt > env->max_try_no)
					status = Service_Failed;
				else
					status = Service_Nonrunning;
				sem_post(&mutex);
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

/**
 * @brief starts the collect process
 *
 * @param env Env struct
 *
 * @return 0 for success, other for failure.
 */
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

/**
 * @brief stop the collect process
 *
 * @return 0 for sucess and other for failure
 */
int stop_collect(){
	kill(collect_pid, SIGKILL);
	return STATUS_OK;
}

/**
 * @brief wrap function for system(3)
 *
 * @param cmd
 *
 * @return -1 on failure, and real exit code on success
 */
static int collect_system(const char* cmd){
	int status;
	errno = 0;
	status = system(cmd);
	if(status == -1)
		return -1;
	else
		return WEXITSTATUS(status);
}

/**
 * @brief get the status of specified service
 *
 * @param env Env struct
 * @param service_index the index of service
 *
 * @return status
 */
static int get_service_status(Env *env,int service_index){
	int i, status;
	for(i = 0; i < MAX_TRY_NUM; i++){
		status = collect_system(env->services[service_index].statecmd);
		if(status != -1)
			return status;
		usleep(10);
	}
	return -1;
}
