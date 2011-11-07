#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>

#include "hast3.h"
#include "communicate.h"
#include "log.h"
#include "collect.h"
#include "function.h"

/* global variables */
Env globalenv;
static sig_atomic_t routine_check_flag = 0;

#define EXIT_BEFORE_UDP		0
#define EXIT_BEFORE_LOG 	1
#define EXIT_BEFORE_CLECT	2
#define EXIT_FINAL	3

int initialize(Env *env);
int server_exit(int exit_level);
void hast3_die(int signum);
void set_routine_check_flag(int signum);
static int main_loop(Env *env);

/* config.c */
int init_config(Env *env, const char *config);

int main(int argc, char const* argv[])
{
	struct itimerval newitimer;
	double value;

	memset(&globalenv, 0, sizeof(Env));
	init_config(&globalenv, "/data/hast3/hast3.conf-custom");
	initialize(&globalenv);

	signal(SIGINT, hast3_die);
	signal(SIGALRM, set_routine_check_flag);

	value = 5 * globalenv.ha_interval;
	newitimer.it_interval.tv_sec = (int )value;
	newitimer.it_interval.tv_usec = (value - (int)value)*1000000;
	newitimer.it_value = newitimer.it_interval;
	setitimer(ITIMER_REAL, &newitimer, NULL);

	exit(main_loop(&globalenv));

	return 0;
}

int initialize(Env *env){
	int status;

	/* start the server */
	status = build_server(env);
	if(status != STATUS_OK){
		fprintf(stderr, "Cannot start the udp server, error code: %d\n",
				status);
		server_exit(EXIT_BEFORE_UDP);
	}

	/* open the log */
	status = open_log(env->logdir);
	if(status != STATUS_OK){
		fprintf(stderr, "Cannot open the log, error code: %d\n", status);
		server_exit(EXIT_BEFORE_LOG);
	}

	/* start the collect process */
	status = start_collect(env);
	if(status != STATUS_OK){
		fprintf(stderr, "Cannot start the collect process, "
				"error code: %d\n", status);
		server_exit(EXIT_BEFORE_CLECT);
	}

	return STATUS_OK;
}

int server_exit(int exit_level){
	switch(exit_level){

		case EXIT_FINAL:
			stop_collect();

		case EXIT_BEFORE_CLECT:
			close_log();

		case EXIT_BEFORE_LOG:
			stop_server(&globalenv);

		case EXIT_BEFORE_UDP:
			break;

		default:
			break;
	}
	if(exit_level == EXIT_FINAL)
		exit(EXIT_SUCCESS);
	else
		exit(EXIT_FAILURE);
	return 0;
}

void set_routine_check_flag(int signum){
	routine_check_flag = 1;
}

void hast3_die(int signum){
	server_exit(EXIT_FINAL);
	exit(EXIT_FAILURE);
}

static int main_loop(Env *env){
	double value;
	struct timeval timeout;
	fd_set readfds, testfds;
	int fd_num = 1, loop=1, result;
	char buf[MAXBUFSIZE];

	FD_ZERO(&readfds);
	FD_SET(env->server_fd, &readfds);
	value = 2 * env->ha_interval;

	while(loop){
		timeout.tv_sec = (int)value;
		timeout.tv_usec = (value - (int)value) * 1000000;
		testfds = readfds;

		result = select(fd_num, &testfds, NULL, NULL, &timeout);
		if(result == -1){
			if(errno == EINTR)
				continue;
		}
		else if(result == 1){
			if(get_and_check_message(env->server_fd, buf) == STATUS_OK)
				dispatch_message(env, buf);
		}
		if(routine_check_flag){
			routine_check_flag = 0;
			routine_check(env);
		}
	}
	
	return STATUS_OK;
}
