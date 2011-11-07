#ifndef _HAST3_LOG_H_
#define _HAST3_LOG_H_

enum log_type{
	DEBUG,
	INFO,
	WARN,
	ERROR,
	FATAL,
	TOTAL_LOG_TYPE
};

#define FLUSH_LOG_NUM	20

#define LOG_NAME "hast3.log"

int write_log(enum log_type type, const char *format, ...);
int open_log(const char *path);
int close_log();

#endif
