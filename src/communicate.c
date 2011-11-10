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
 * @file communicate.c
 * @brief 
 * @author Li Jiliang<tjulijiliang@gmail.com
 * @version 1.0
 * @date 2011-11-09
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>

#include "hast3.h"
#include "communicate.h"

/**
 * @brief creates the udp socket to receive multicast information
 *
 * @param env Env struct
 *
 * @return STATUS_OK on success, STATUS_SOCKET_ERR on failure
 */
int build_server(Env *env){
	struct sockaddr_in addr;
	struct ip_mreq mreq;

	u_int yes=1;

	/* create what looks like an ordinary UDP socket */
	if ((env->server_fd=socket(AF_INET,SOCK_DGRAM,0)) < 0) {
		fprintf(stderr, "socket error");
		return STATUS_SOCKET_ERR;
	}


	/* allow multiple sockets to use the same PORT number */
	if (setsockopt(env->server_fd,SOL_SOCKET,SO_REUSEADDR,&yes,
				sizeof(yes)) < 0) {
		perror("Reusing ADDR failed");
		return STATUS_SOCKET_ERR;
	}

	/* set up destination address */
	memset(&addr,0,sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=htonl(INADDR_ANY);
	addr.sin_port=htons((uint16_t)env->port);

	/* bind to receive address */
	if (bind(env->server_fd,(struct sockaddr *) &addr,sizeof(addr)) < 0) {
		fprintf(stderr, "bind error");
		return STATUS_SOCKET_ERR;
	}

	/* use setsockopt() to request that the kernel join a multicast group */
	mreq.imr_multiaddr.s_addr=inet_addr(HEARTBEAT_GROUP);
	mreq.imr_interface.s_addr=htonl(INADDR_ANY);
	if (setsockopt(env->server_fd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,
				sizeof(mreq)) < 0) {
		fprintf(stderr, "Failed to enter multicast group");
		return STATUS_SOCKET_ERR;
	}

	return STATUS_OK;
}

/**
 * @brief stop the udp server
 *
 * @param env Env struct
 *
 * @return 0 on success, other on failure
 */
int stop_server(Env *env){
	return close(env->server_fd);
}

/**
 * @brief compute the CRC checksum
 *
 * @param addr message address
 * @param len message length
 *
 * @return checksum
 */
u_short checksum(u_short* addr, int len){
	register int left = len;
	register int sum = 0;
	u_short answer = 0;

	while(left > 1){
		sum += *addr++;
		left -= 2;
	}

	/* add left-over byte if necessary */
	if(left == 1)
		sum += *(unsigned char *)addr;
	
	/* add carries from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum &0xffff);
	/* add possible carry */
	sum += (sum >> 16);

	answer = ~sum;
	return answer;
}

/**
 * @brief when the socket is readble, read the socket and checks its validity
 *
 * @param fd udp socket
 * @param buf[] where the data should be stored
 *
 * @return STATUS_OK on success and STATUS_MSG_CORRUPT on failure
 */
int get_and_check_message(int fd, char buf[]){
	struct sockaddr_in addr;
	int len = 0, addrlen = sizeof(addr), header_len, entry_len;

	if(ioctl(fd, FIONREAD, &len) != -1 && len <= 0)
		return STATUS_MSG_CORRPUT;

	len = recvfrom(fd, buf, MAXBUFSIZE, 0, 
			(struct sockaddr *)&addr, &addrlen);
	if(len < 0)
		return STATUS_MSG_CORRPUT;

	/* check the checksum */
	if(checksum((u_short *)buf, len) != 0)
		return STATUS_MSG_CORRPUT;

	header_len = sizeof(Hast3_message);
	entry_len = sizeof(Hast3_message_entry);
	if((len - header_len) % entry_len != 0)
		return STATUS_MSG_CORRPUT;

	return STATUS_OK;
}

/**
 * @brief send a command to the specified node, i.e., ask it to start or stop the service
 *
 * @param env Env struct
 * @param node name of the receiving node
 * @param service name of the service
 * @param cmd command
 *
 * @return STATUS_OK on success and STATUS_SOCKET_ERR on failure
 */
int send_cmd_to_node(Env *env, const char*node, const char *service, int cmd){
	int fd, retry, sent;
	unsigned sndcnt;
	struct sockaddr_in address;
	struct hostent *hostinfo;
	size_t addrlen = sizeof(address), msglen;
	Hast3_message *msg;
	Hast3_message_entry *entry;

	msglen = sizeof(Hast3_message) + sizeof(Hast3_message_entry);
	msg = (Hast3_message *)malloc(msglen);
	if(msg == NULL)
		return STATUS_SOCKET_ERR;
	strcpy(msg->nodename, env->nodename);
	msg->type = HAST3_MSG_CMD;
	msg->field_num = 1;
	entry = (Hast3_message_entry*)((char *)msg + sizeof(Hast3_message));
	strcpy(entry->service_name, service);
	entry->cmd_or_status = (short)cmd;
	msg->checksum = 0;
	msg->checksum = checksum((u_short *)msg, msglen);

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if(fd < 0){
		free(msg);
		return STATUS_SOCKET_ERR;
	}

	hostinfo = gethostbyname(node);
	if(hostinfo == NULL){
		close(fd);
		free(msg);
		return STATUS_SOCKET_ERR;
	}

	memset(&address, 0, addrlen);
	address.sin_family = AF_INET;
	address.sin_port = htons(env->port);
	address.sin_addr = *(struct in_addr *)*hostinfo->h_addr_list;

	sndcnt = 0;
	retry = 0;
	while(retry < RETRYCNT && sndcnt < msglen){
		sent = sendto(fd, msg, msglen, 0, 
				(struct sockaddr *)&address, addrlen);
		if(sent < 0)
			retry++;
		else
			sndcnt += sent;
	}

	free(msg);
	close(fd);

	if(retry < RETRYCNT)
		return STATUS_OK;
	else
		return STATUS_SOCKET_ERR;
}
