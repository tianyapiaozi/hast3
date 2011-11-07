#include <stdlib.h>
#include <string.h>

#include "hast3.h"
#include "log.h"
#include "communicate.h"

#define STATUS_TABLE_RESIZE	10

int sort_status_table(Env *env);
int remove_dead_nodes(Env *env);
int cmp_active_node(const void *arg1, const void *arg2);
int sort_status_table(Env *env);
int service_shift(Env *env, const char *out_node, const char *in_node, int service);
int routine_check(Env *env);
int resize_statue_table(Env *env);
int free_active_node(Active_node *node);
Active_node * malloc_active_node(Env *env);
int update_status_table(Env *env, Hast3_message *msg, Hast3_message_entry *entries);
void stop_service(Env *env, int service_index);
void start_service(Env *env, int service_index);
int deal_service(Env* env, Hast3_message *msg, Hast3_message_entry *entry);
int dispatch_message(Env* env, char *buf);

int dispatch_message(Env* env, char *buf){
	Hast3_message *ptr = (Hast3_message *)buf;
	int i;

	if(ptr->type == HAST3_MSG_CMD)
		for(i = 0; i < ptr->field_num; i++)
			deal_service(env, ptr, &ptr->data[i]);
	else if(ptr->type == HAST3_MSG_BCAST)
		update_status_table(env, ptr, ptr->data);
	else{
		/*TODO log*/;
	}
	return STATUS_OK;
}

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

void start_service(Env *env, int service_index){
	int ind = service_index, i;
	for(i = 0; i < env->active_node_num; i++)
		if(strcmp(env->nodes[i]->nodename, env->nodename) == 0){
			if(env->nodes[i]->statues[service_index] == Service_Running)
				return;
			else
				break;
		}

	do{
		if(system(env->services[ind].startcmd) == 0){
			env->services[ind].tried_cnt = 0;
			break;
		}
		env->services[ind].tried_cnt++;
	} while(env->services[ind].tried_cnt <= env->max_try_no);
}

void stop_service(Env *env, int service_index){
	int ind = service_index, i;
	for(i = 0; i < env->active_node_num; i++)
		if(strcmp(env->nodes[i]->nodename, env->nodename) == 0){
			if(env->nodes[i]->statues[service_index] != Service_Running)
				return;
			else
				break;
		}

	for(i = 0; i < MAX_TRY_NUM; i++)
		if(system(env->services[ind].stopcmd) == 0)
			break;
}

int update_status_table(Env *env, Hast3_message *msg, Hast3_message_entry *entries){
	int i, j;
	for(i = 0; i < env->active_node_num; i++)
		/* update the entry */
		if(strcmp(env->nodes[i]->nodename, msg->nodename) == 0){
			for(j = 0; j < env->service_num; j++)
				env->nodes[i]->statues[j] = entries[j].cmd_or_status;
			time(&env->nodes[i]->last_update);
			return 0;
		}

	/* insert the new entry */
	if(i >= env->active_node_num){
		/* more memory should be malloced */
		if(env->active_node_num >= env->active_node_cap){
			/*TODO check status */
			resize_statue_table(env);
		}
		i = env->active_node_num;
		env->nodes[i] = malloc_active_node(env);
		if(env->nodes[i] != NULL){
			strcpy(env->nodes[i]->nodename, msg->nodename);
			for(j = 0; j < env->service_num; j++)
				env->nodes[i]->statues[j] = entries[j].cmd_or_status;
			time(&env->nodes[i]->last_update);
			env->active_node_num++;
		}
		else{
			/*TODO */
		}
		return 0;
	}
}

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

int free_active_node(Active_node *node){
	free(node->statues);
	free(node);
	return 0;
}

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

int routine_check(Env *env){
	int active_node_num, i, j, k;
	Active_node ** nodes;
	int *statues;
	/* none or multiple service(s) flag */
	int mul_or_none_flag = 0;

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
			send_cmd_to_node(env, nodes[active_node_num-1]->nodename,
							 env->services[i].name,
							 HAST3_CMD_START);
			write_log(INFO, "Tell node [%s] to START service [%s]",
					nodes[active_node_num-1]->nodename,
					env->services[i].name);
		}
		/* multiple services are running */
		else{
			for(j = active_node_num-1; j >= 0; j--)
				if(nodes[j]->statues[i] == Service_Running)
					break;
			for(j--; j >= 0; j--)
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
		i = 0;
		j = active_node_num - 1;
		if(i < j && nodes[i]->service_cnt > nodes[j]->service_cnt + 2){
			for(k = 0; k < env->service_num; k++)
				if(nodes[i]->statues[k] == Service_Running){
					service_shift(env, nodes[i]->nodename, 
							  nodes[j]->nodename, k);
					break;
				}
		}
	}

	free(statues);
	return 0;
}

int service_shift(Env *env, const char *out_node, const char *in_node, int service){
	write_log(INFO, "Shifting service [%s] from node [%s] to node [%s]",
			env->services[service].name, out_node, in_node);
	send_cmd_to_node(env, out_node, env->services[service].name, HAST3_CMD_STOP);
	send_cmd_to_node(env, in_node, env->services[service].name, HAST3_CMD_START);
	return 0;
}

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
