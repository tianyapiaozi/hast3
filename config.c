#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "hast3.h"
#include "keyfile.h"

int init_config(Env *env, const char *config){
	Keyfile *keyfile;
	int		integer, i;
	char	servicex[] = "ServiceX";
	char	str[MAXSTRLEN];
	double	value;

	/* get the absolute path of the config */
	if (realpath(config, env->config) == NULL)
		return STATUS_CNF_ERR;
	
	/* start reading the config */
	if (initKeyfile(&keyfile, env->config) != 0) 
		return STATUS_CNF_ERR;

	/* the general section: */
	getStrValue(keyfile, "General", "NodeName", str);
	strcpy(env->nodename, str);
	if(strlen(env->nodename) >= NAMELEN){
		env->nodename[NAMELEN-1] = '\0';
		fprintf(stderr, "[WARNING]\tTruncate node name %s to %s"
				"(at most %d is alowed)\n", str, env->nodename, NAMELEN-1);
	}

	getStrValue(keyfile, "General", "LogDir", str);
	if (access(str, R_OK|W_OK|X_OK) == 0)	
		strcpy(env->logdir, str);
	else{
		fprintf(stderr, "LogDir %s has wrong permissions\n", str);
		return STATUS_CNF_ERR;
	}

	/* The port number should be none well know, i.e. greater than 1024 */
	getIntValue(keyfile, "General", "Port", &integer);
	if (integer > 1024 && integer < 65536)
		env->port = integer;
	else
		env->port = 10010; /* default to 10086 */

	/* the runtime part */
	getFloatValue(keyfile, "Runtime", "HAInterval", &value);
	if(value > 0.0)
		env->ha_interval = value;
	else{
		fprintf(stderr, "HAInterval should be greater than 0.0\n");
		return STATUS_CNF_ERR;
	}

	getIntValue(keyfile, "Runtime", "DeadTime", &integer);
	if(integer > env->ha_interval)
		env->dead_time = integer;
	else{
		fprintf(stderr, "DeadTime should be greater than HAInterval\n");
		return STATUS_CNF_ERR;
	}

	getIntValue(keyfile, "Runtime", "ServiceNumber", &integer);
	if(integer > 0)
		env->service_num = integer;
	else{
		fprintf(stderr, "Number of services should be greater than 0\n");
		return STATUS_CNF_ERR;
	}

	getIntValue(keyfile, "Runtime", "MaxTryNum", &integer);
	if(integer > 0)
		env->max_try_no = integer;
	else{
		fprintf(stderr, "MaxTryNum should be greater than 0\n");
		return STATUS_CNF_ERR;
	}

	/* Initialize the services */
	env->services = calloc(env->service_num, sizeof(Service));
	for(i = 0; i < env->service_num; i++){
		servicex[7] = '0' + i;
		getStrValue(keyfile, servicex, "ServiceName", str);
		strcpy(env->services[i].name, str);
		if(strlen(env->services[i].name) >= NAMELEN){
			env->services[i].name[NAMELEN-1] = '\0';
			fprintf(stderr, "[WARNING]\tTruncate service name %s to %s"
					"(at most %d is alowed)\n", str, env->services[i].name, 
					NAMELEN-1);
		}

		getStrValue(keyfile, servicex, "StartCMD", str);
		strcpy(env->services[i].startcmd, str);
//		if(access(str, R_OK | X_OK) == 0)
//			strcpy(env->services[i].startcmd, str);
//		else{
//			fprintf(stderr, "Wrong permissions with %s\n", str);
//			return STATUS_CNF_ERR;
//		}

		getStrValue(keyfile, servicex, "StopCMD", str);
		strcpy(env->services[i].stopcmd, str);
//		if(access(str, R_OK | X_OK) == 0)
//			strcpy(env->services[i].stopcmd, str);
//		else{
//			fprintf(stderr, "Wrong permissions with %s\n", str);
//			return STATUS_CNF_ERR;
//		}

		getStrValue(keyfile, servicex, "StateCMD", str);
		strcpy(env->services[i].statecmd, str);
//		if(access(str, R_OK | X_OK) == 0)
//			strcpy(env->services[i].statecmd, str);
//		else{
//			fprintf(stderr, "Wrong permissions with %s\n", str);
//			return STATUS_CNF_ERR;
//		}

		env->services[i].tried_cnt = 0;
	}

	destroyKeyfile(keyfile);
	return STATUS_OK;
}
