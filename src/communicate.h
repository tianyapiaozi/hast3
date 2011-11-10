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
