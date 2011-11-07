#ifndef _COMMUNICATE_H_
#define _COMMUNICATE_H_

#include <sys/types.h>

#include "hast3.h"

#define RETRYCNT 5

#define HEARTBEAT_GROUP "225.0.0.37"

u_short checksum(u_short* addr, int len);

int send_cmd_to_node(Env *env, const char*node, const char *service, int cmd);
int build_server(Env *env);
int stop_server(Env *env);
int get_and_check_message(int fd, char buf[]);


#endif
