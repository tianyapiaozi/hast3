#ifndef _HAST3_H_
#define _HAST3_H_
#include <limits.h>
#include <time.h>

#include "log.h"

#define MAXSTRLEN 1024
#define MAXBUFSIZE 2048
#define MAXFILENAMELEN PATH_MAX
#define NAMELEN 16

typedef struct{
	char name[NAMELEN];
	char startcmd[MAXSTRLEN];
	char stopcmd[MAXSTRLEN];
	char statecmd[MAXSTRLEN];
	int tried_cnt;
} Service;

typedef struct Active_node{
	char nodename[NAMELEN];
	int *statues;
	time_t last_update;
	int service_cnt;
} Active_node;

typedef struct{
	double ha_interval;
	int  dead_time;
	int max_try_no;
	int port;
	int server_fd;
	char config[MAXFILENAMELEN];
	char logdir[MAXFILENAMELEN];
	char nodename[NAMELEN];
	int service_num;
	Service *services;
	int active_node_num;
	int active_node_cap;
	Active_node** nodes;
} Env;


/* The return status code */
#define STATUS_OK			0
#define STATUS_CNF_ERR		1
#define STATUS_SOCKET_ERR	2
#define STATUS_CLECT_ERR	3
#define STATUS_LOG_ERR		4
#define STATUS_SVR_ERR		5
#define STATUS_MSG_CORRPUT	6
#define STATUS_CMD_ERR		7

enum Service_status{
	Service_Running=0,
	Service_Nonrunning,
	Service_Failed
};

#define HAST3_MSG_BCAST	0
#define HAST3_MSG_CMD	1

#define HAST3_CMD_START	0
#define HAST3_CMD_STOP	1

typedef struct {
	char service_name[NAMELEN];
	short cmd_or_status;
} Hast3_message_entry;

typedef struct {
	char nodename[NAMELEN];
	short type;
	short field_num;
	unsigned short checksum;
	Hast3_message_entry data[0];
} Hast3_message;

#define MAX_TRY_NUM 3

#endif
