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
 * @file function.c
 * @brief 
 * @author Li Jiliang<tjulijiliang@gmail.com
 * @version 1.0
 * @date 2011-11-09
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <semaphore.h>

#include "hast3.h"
#include "log.h"
#include "communicate.h"
#include "function.h"
#include "util.h"

#define STATUS_TABLE_RESIZE	10

int sort_status_table(Env *env);
int remove_dead_nodes(Env *env);
int cmp_active_node(const void *arg1, const void *arg2);
int sort_status_table(Env *env);
int service_shift(Env *env, const char *out_node, const char *in_node, int service);
int resize_statue_table(Env *env);
int free_active_node(Active_node *node);
Active_node * malloc_active_node(Env *env);
int update_status_table(Env *env, Hast3_message *msg, Hast3_message_entry *entries);
void stop_service(Env *env, int service_index);
void start_service(Env *env, int service_index);
int get_status(Env *env,int service_index);
int deal_service(Env* env, Hast3_message *msg, Hast3_message_entry *entry);

extern sem_t mutex;

/**
 * @brief perform suitable actions according to the message type, i.e. update the status table if it's a broadcast message and executes the command if it's a command message
 *
 * @param env Env struct
 * @param buf address of the message
 *
 * @return STATUS_OK
 */
int dispatch_message(Env* env, const char *buf){
	Hast3_message *ptr = (Hast3_message *)buf;
	int i;

	if(ptr->type == HAST3_MSG_CMD)
		for(i = 0; i < ptr->field_num; i++)
			deal_service(env, ptr, &ptr->data[i]);
	else if(ptr->type == HAST3_MSG_BCAST)
		update_status_table(env, ptr, ptr->data);
	else{
		write_log(WARN, "Ignore malformed message");
	}
	return STATUS_OK;
}

/**
 * @brief executes the command
 *
 * @param env Env struct
 * @param msg message header
 * @param entry message entry
 *
 * @return STATUS_OK on success and STATUS_CMD_ERR on failure
 */
int deal_service(Env* env, Hast3_message *msg, Hast3_message_entry *entry){
	int i;
	for(i = 0; i < env->service_num; i++)
		if(strcmp(entry->service_name, env->services[i].name) == 0)
			break;
	if(i < env->service_num){
		if(entry->cmd_or_status == HAST3_CMD_START){
			write_log(INFO, "Get CMD from node [%s] to start service [%s]",
					msg->nodename, entry->service_name);
			start_service(env, i);
		}
		else if(entry->cmd_or_status == HAST3_CMD_STOP){
			write_log(INFO, "Get CMD from node [%s] to stop service [%s]",
					msg->nodename, entry->service_name);
			stop_service(env, i);
		}
		else{
			write_log(ERROR, "Unknown CMD from node [%s]",
					msg->nodename);
			return STATUS_CMD_ERR;
		}
	}
	else{
		write_log(ERROR, "Unknown service [%s] from node [%s]",
				entry->service_name, msg->nodename);
		return STATUS_CMD_ERR;
	}
	return STATUS_OK;
}

/**
 * @brief start the service if it's not already started
 *
 * @param env Env struct 
 * @param service_index the index of the service
 */
void start_service(Env *env, int service_index){
	int ind = service_index, status;

	/* if the service is already running, return imediately */
	status = get_status(env, service_index);
	if(status == 0)
		return;

	sem_wait(&mutex);
	do{
		if(wrap_system(env->services[ind].startcmd) == 0){
			env->services[ind].tried_cnt = 0;
			break;
		}
		env->services[ind].tried_cnt++;
	} while(env->services[ind].tried_cnt <= env->max_try_no);
	sem_post(&mutex);
}

/**
 * @brief stop the service if it's running
 *
 * @param env Env struct
 * @param service_index the index of the service
 */
void stop_service(Env *env, int service_index){
	int ind = service_index, i;

	/* if the service is not running, return imediately */
	if(get_status(env, service_index) != 0)
		return;

	for(i = 0; i < MAX_TRY_NUM; i++)
		if(wrap_system(env->services[ind].stopcmd) == 0)
			break;
}

/**
 * @brief get the status of specified service
 *
 * @param env Env struct
 * @param service_index the index of the service
 *
 * @return the status on success and -1 on failure
 */
int get_status(Env *env,int service_index){
	int i, status;
	for(i = 0; i < MAX_TRY_NUM; i++){
		status = wrap_system(env->services[service_index].statecmd);
		if(status != -1)
			return status;
		usleep(10);
	}
	return -1;
}

/**
 * @brief update the status table
 *
 * @param env Env struct
 * @param msg message header
 * @param entries message entries
 *
 * @return 0 on success and 1 on failure
 */
int update_status_table(Env *env, Hast3_message *msg, Hast3_message_entry *entries){
	int i, j;
	for(i = 0; i < env->active_node_num; i++)
		/* update the entry */
		if(strcmp(env->nodes[i]->nodename, msg->nodename) == 0){
			if(debug_level > 0){
				write_log(DEBUG, "Update status info of node [%s]",
						msg->nodename);
			}

			for(j = 0; j < env->service_num; j++)
				env->nodes[i]->statues[j] = entries[j].cmd_or_status;
			time(&env->nodes[i]->last_update);
			return 0;
		}

	/* insert the new entry */
	if(i >= env->active_node_num){
		/* more memory should be malloced */
		if(env->active_node_num >= env->active_node_cap){
			/* if fails to resize status table, return imediately */
			if(resize_statue_table(env) != 0){
				write_log(ERROR, "Failed to resize status table");
				return 1;
			}
		}
		i = env->active_node_num;
		env->nodes[i] = malloc_active_node(env);
		if(env->nodes[i] != NULL){
			if(debug_level > 0){
				write_log(DEBUG, "Insert status info of node [%s]",
						msg->nodename);
			}

			strcpy(env->nodes[i]->nodename, msg->nodename);
			for(j = 0; j < env->service_num; j++)
				env->nodes[i]->statues[j] = entries[j].cmd_or_status;
			time(&env->nodes[i]->last_update);
			env->active_node_num++;
		}
		/* malloc new nodes failed */
		else{
			write_log(ERROR, "Failed to malloc new Active_node");
			return 1;
		}
		return 0;
	}
}

/**
 * @brief malloc a Active_node struct
 *
 * @param env Env struct
 *
 * @return address on success and NULL on failure
 */
Active_node * malloc_active_node(Env *env){
	Active_node *tmp = (Active_node *)calloc(1, sizeof(Active_node));
	if(tmp != NULL){
		tmp->statues = (int *)calloc(env->service_num, sizeof(int));
		if(tmp->statues != NULL)
			return tmp;
		else{
			free(tmp);
			return NULL;
		}
	}
	else
		return NULL;
}

/**
 * @brief free the Active_node struct
 *
 * @param node pointer to Active_node struct
 *
 * @return 0
 */
int free_active_node(Active_node *node){
	free(node->statues);
	free(node);
	return 0;
}

/**
 * @brief resize the status table
 *
 * @param env Env struct
 *
 * @return 0 on success and 1 on failure
 */
int resize_statue_table(Env *env){
	Active_node **tmp = env->nodes;
	int new_compacity = env->active_node_cap + STATUS_TABLE_RESIZE;

	tmp = realloc(tmp, new_compacity * sizeof(Active_node *));
	if(tmp == NULL)
		return 1;
	env->nodes = tmp;
	env->active_node_cap = new_compacity;

	return 0;
}

/**
 * @brief scans the status table and decides to start or stop service on some node
 *
 * @param env Env struct
 *
 * @return 0 on success
 */
int routine_check(Env *env){
	int active_node_num, i, j, k;
	Active_node ** nodes;
	int *statues;

	/* none or multiple service(s) flag */
	int mul_or_none_flag = 0;
	/* break all loop flag */
	int break_all = 0;

	remove_dead_nodes(env);
	sort_status_table(env);

	nodes = env->nodes;
	active_node_num = env->active_node_num;
	if(active_node_num <= 0)
		return 0;

	statues = (int *)calloc(env->service_num, sizeof(int));
	if(statues == NULL)
		return 1;

	for(i = 0; i < env->service_num; i++)
		for(j = 0; j < active_node_num; j++)
			if(nodes[j]->statues[i] == Service_Running)
				statues[i]++;
	for(i = 0; i < env->service_num; i++)
		if(statues[i] != 1){
			mul_or_none_flag = 1;
			break;
		}

	if(mul_or_none_flag){
		/* none service i is running */
		if(statues[i] == 0){
			/* 
			 * find the node who has the lowest load and whose status of
			 * service i is not Service_Failed, then ask it to start
			 * service i
			 */
			for(j = 0; j < active_node_num; j++)
				if(env->nodes[j]->statues[i] != Service_Failed)
					break;

			send_cmd_to_node(env, nodes[j]->nodename,
							 env->services[i].name,
							 HAST3_CMD_START);
			write_log(INFO, "Tell node [%s] to START service [%s]",
					nodes[j]->nodename,
					env->services[i].name);
		}
		/* multiple services are running */
		else{
			/* 
			 * find the node who has the lowest load and whose service i
			 * is running
			 */
			for(j = 0; j < active_node_num; j++)
				if(nodes[j]->statues[i] == Service_Running)
					break;

			/* 
			 * All the other nodes whose service i is running should be 
			 * commanded to shutdown their service i
			 */
			for(j++; j < active_node_num; j++)
				if(nodes[j]->statues[i] == Service_Running){
					send_cmd_to_node(env, nodes[j]->nodename,
							         env->services[i].name,
									 HAST3_CMD_STOP);
					write_log(INFO, "Tell node [%s] to STOP service [%s]",
							nodes[j]->nodename,
							env->services[i].name);
				}
		}
	}
	/* 
	 * All the services are running with one and only one instance,
	 * now check if service shift is needed 
	 */
	else{
		break_all = 0;
		for(i = 0; i < active_node_num; i++){
			for(j = active_node_num-1; i < j && nodes[i]->service_cnt + 
					2 <= nodes[j]->service_cnt; j--){
				for(k = 0; k < env->service_num; k++){
					if(nodes[j]->statues[k] == Service_Running &&
							nodes[i]->statues[k] != Service_Failed){
						service_shift(env, nodes[j]->nodename, 
								nodes[i]->nodename, k);
						break_all = 1;
						break;
					}
				}
				if(break_all)
					break;
			}
			if(break_all)
				break;
		}
	}

	free(statues);
	return 0;
}

/**
 * @brief shift a service from one node to another
 *
 * @param env Env struct
 * @param out_node name of the out node
 * @param in_node name of the in node
 * @param service the index of the service
 *
 * @return 0
 */
int service_shift(Env *env, const char *out_node, const char *in_node, int service){
	write_log(INFO, "Shifting service [%s] from node [%s] to node [%s]",
			env->services[service].name, out_node, in_node);
	send_cmd_to_node(env, out_node, env->services[service].name, HAST3_CMD_STOP);
	send_cmd_to_node(env, in_node, env->services[service].name, HAST3_CMD_START);
	return 0;
}

/**
 * @brief sort the status table in ascending order according to total service running in the node and node name
 *
 * @param env Env struct
 *
 * @return 0
 */
int sort_status_table(Env *env){
	int active_node_num, i, j, count;
	Active_node **nodes;

	nodes = env->nodes;
	active_node_num = env->active_node_num;
	for(i = 0; i < active_node_num; i++){
		count = 0;
		for(j = 0; j < env->service_num; j++)
			if(nodes[i]->statues[j] == Service_Running)
				count++;
		nodes[i]->service_cnt = count;
	}
	qsort(nodes, active_node_num, sizeof(Active_node *), cmp_active_node);
	return 0;
}

/**
 * @brief compares two Active_node
 *
 * @param arg1 pointer to Active_node
 * @param arg2 pointer to Active_node
 *
 * @return 1 if greater, 0 if equal, -1 otherwise
 */
int cmp_active_node(const void *arg1, const void *arg2){
	Active_node *pnode1 = *(Active_node **)arg1;
	Active_node *pnode2 = *(Active_node **)arg2;
	if(pnode1 -> service_cnt < pnode2 -> service_cnt)
		return -1;
	else if(pnode1 -> service_cnt > pnode2 ->service_cnt)
		return 1;
	else
		return strcmp(pnode1->nodename, pnode2->nodename);
}

/**
 * @brief remove the nodes who haven't annonced for dead_time interval
 *
 * @param env Env struct
 *
 * @return number of deleted items
 */
int remove_dead_nodes(Env *env){
	int active_node_num, i, j, delete_cnt=0;
	Active_node **nodes;
	time_t now;

	time(&now);
	active_node_num = env->active_node_num;
	nodes = env->nodes;
	for(i = 0; i < active_node_num; ){
		if(nodes[i]->last_update < now - env->dead_time){
			write_log(INFO, "Node: [%s] inactive, delete it now", 
					nodes[i]->nodename);
			free_active_node(nodes[i]);
			delete_cnt++;
			for(j = i+1; j < active_node_num; j++)
				env->nodes[j-1] = env->nodes[j];
			active_node_num--;
		}
		else
			i++;
	}
	env->active_node_num = active_node_num;
	return delete_cnt;
}
