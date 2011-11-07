#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>

#include "hast3.h"
#include "log.h"

static FILE *log_fp = NULL;
static char log_names[TOTAL_LOG_TYPE][7] = 
		{"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
static int log_size = 0;

int open_log(const char *path){
	char logpath[FILENAME_MAX];
	sprintf(logpath, "%s/%s", path, LOG_NAME);

	log_fp = fopen(logpath, "a");
	if(log_fp == NULL)
		return STATUS_LOG_ERR;
	write_log(INFO, "Open the logfile by PID: %d", getpid());
	return STATUS_OK;
}

int write_log(enum log_type type, const char *format, ...){
	char log_entry[MAXBUFSIZE];
	va_list arg_ptr;
	int len;
	time_t now;

	va_start(arg_ptr, format);
	len = vsprintf(log_entry, format, arg_ptr);
	va_end(arg_ptr);

	time(&now);
	fprintf(log_fp, "[%s] %s\t%s\n", log_names[type], ctime(&now),
			log_entry);
	
	if(++log_size % FLUSH_LOG_NUM == 0){
		log_size = 0;
		fflush(log_fp);
	}

	return STATUS_OK;
}

int close_log(){
	fclose(log_fp);
	return STATUS_OK;
}
