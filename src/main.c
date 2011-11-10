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
 * @file main.c
 * @brief 
 * @author Li Jiliang<tjulijiliang@gmail.com
 * @version 1.0
 * @date 2011-11-09
 */
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <semaphore.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/mman.h>

#include "hast3.h"
#include "communicate.h"
#include "log.h"
#include "collect.h"
#include "function.h"

/* global lock */
sem_t mutex;
/* global variables */
Env *env;
int debug_level = 0;
static sig_atomic_t routine_check_flag = 0;

#define EXIT_BEFORE_UDP		0
#define EXIT_BEFORE_LOG 	1
#define EXIT_BEFORE_CLECT	2
#define EXIT_FINAL	3

static void show_usage();
static void show_version();
static void free_runtime_mem();

int initialize(Env *env);
int server_exit(int exit_level);
void hast3_die(int signum);
void set_routine_check_flag(int signum);
static int main_loop();

/* config.c */
int init_config(Env *env, const char *config);

int main(int argc, char *argv[])
{
	struct itimerval newitimer;
	double value;
	int daemon_flag = 1;
	int opt;
	char config[MAXFILENAMELEN] = "/etc/hast3/hast3.conf-custom";
	char shortopt[] = "bfc:dhv";
	struct option longopt[] = {
		{"background",	no_argument,		NULL,	'b'},
		{"foreground",	no_argument,		NULL,	'f'},
		{"config",		required_argument,	NULL,	'c'},
		{"debug",		no_argument,		NULL,	'd'},
		{"help",		no_argument,		NULL,	'h'},
		{"version",		no_argument,		NULL,	'v'},
		{0,				0,					0,		0},
	};

	optind = 1;
	opterr = 0;

	while((opt = getopt_long(argc, argv, shortopt, longopt, NULL)) != EOF){
		switch(opt){
			case 'b':
				/* Run in the background */
				daemon_flag = 1;
				break;

			case 'f':
				/* run in the foreground */
				daemon_flag = 0;
				break;

			case 'c':
				strcpy(config, optarg);
				break;

			case 'd':
				if((optind < argc) && *argv[optind] != '-'){
					debug_level = atoi(argv[optind]);
					optind++;
				}
				else{
					debug_level = 1;
				}
				break;

			case 'v':
				show_version();
				return 0;

			case 'h':
				show_usage();
				return 0;

			default:
				show_usage();
				return 1;
		}
	}

	/* get the shared memory */
	env = mmap(NULL, sizeof(Env), PROT_READ | PROT_WRITE, 
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	if(env == MAP_FAILED){
		fprintf(stderr, "cannot get the shared memroy");
		exit(EXIT_FAILURE);
	}

	/* initialize the semaphore that is shared between processes */
	sem_init(&mutex, 1, 1);

	/* initialize the globalenv struct according to config file */
	memset(env, 0, sizeof(Env));
	init_config(env, config);

	if(daemon_flag)
		daemon(0, 0);

	/* system initialization */
	initialize(env);

	signal(SIGINT, hast3_die);
	//signal(SIGCHLD, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
	signal(SIGTTOU, SIG_IGN);

	/* set up SIGALRM signal */
	signal(SIGALRM, set_routine_check_flag);

	/* performs routine check every 5 * ha_interval */
	value = 5 * env->ha_interval;
	newitimer.it_interval.tv_sec = (int )value;
	newitimer.it_interval.tv_usec = (value - (int)value)*1000000;
	newitimer.it_value = newitimer.it_interval;
	setitimer(ITIMER_REAL, &newitimer, NULL);

	/* go to the main loop */
	exit(main_loop());

	return 0;
}

/**
 * @brief system initialization
 *
 * @param env Env struct
 *
 * @return STATUS_OK on success
 */
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

/**
 * @brief clean up according to exit level and then exit
 *
 * @param exit_level exit level
 *
 * @return 0
 */
int server_exit(int exit_level){
	switch(exit_level){

		case EXIT_FINAL:
			stop_collect();
			free_runtime_mem();

		case EXIT_BEFORE_CLECT:
			close_log();

		case EXIT_BEFORE_LOG:
			stop_server(env);

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

/**
 * @brief set the routine_check_flag
 *
 * @param signum signal number
 */
void set_routine_check_flag(int signum){
	routine_check_flag = 1;
}

/**
 * @brief system exit
 *
 * @param signum signal number
 */
void hast3_die(int signum){
	server_exit(EXIT_FINAL);
	exit(EXIT_FAILURE);
}

/**
 * @brief system main loop
 *
 * @return loop forever and no return
 */
static int main_loop(){
	double value;
	struct timeval timeout;
	fd_set readfds, testfds;
	int loop=1, result;
	char buf[MAXBUFSIZE];

	FD_ZERO(&readfds);
	FD_SET(env->server_fd, &readfds);
	value = 2 * env->ha_interval;

	while(loop){
		timeout.tv_sec = (int)value;
		timeout.tv_usec = (value - (int)value) * 1000000;
		testfds = readfds;

		result = select(FD_SETSIZE, &testfds, NULL, NULL, &timeout);
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

/**
 * @brief show the version information
 */
static void show_version(){
#if defined(__HAST3_BUILD_DATE__) && defined(__HAST3_BUILD_ARCH__)
	printf("hast3 version 1.0 (build "__HAST3_BUILD_DATE__") for "
			__HAST3_BUILD_ARCH__"\n");
#else
	printf("hast3 version 1.0\n");
#endif
}

/**
 * @brief show the usage information
 */
static void show_usage(){
	show_version();
	printf("\nUsage: hast3 [OPTION] ...\n");
	printf("Start Hast3(Heartbeat for AST3)\n\n");

	printf("Options:\n");
	printf(" -b, --background\trunning in background(by default)\n");
	printf(" -f, --foreground\trunning in foreground\n");
	printf(" -c, --config\t\tspecify the config file path\n");
	printf(" -d, --debug\t\tspecify the debug level(default to 1)\n");
	printf(" -v, --version\t\tshow version infomation\n");
	printf(" -h, --help\t\tshow version and usage\n\n");

	printf("Please report bugs to <tjulijiliang@gmail.com>\n");
}

/**
 * @brief clean up resources
 */
static void free_runtime_mem(){
	int i;

	/* free the status table staff */
	for(i = 0; i < env->active_node_num; i++){
		free(env->nodes[i]->statues);
		free(env->nodes[i]);
	};
	free(env->nodes);

	/* free the services */
	free(env->services);

	/* destroy the mutex */
	sem_destroy(&mutex);

	/* unmap the shared memory */
	munmap(env, 0);
}
