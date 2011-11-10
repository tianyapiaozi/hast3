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
 * @file log.c
 * @brief 
 * @author Li Jiliang<tjulijiliang@gmail.com
 * @version 1.0
 * @date 2011-11-09
 */
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "hast3.h"
#include "log.h"
#include "util.h"

/* static functions */
static int compress(const char *path);
static int decompress(const char *path);

static FILE *log_fp = NULL;
static char log_names[TOTAL_LOG_TYPE][7] = 
		{"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
static int log_size = 0;
static time_t next_day_mark;
static char logpath[FILENAME_MAX];

/**
 * @brief open the log file
 *
 * @param path log directory
 *
 * @return STATUS_OK on success and STATUS_LOG_ERR on failure
 */
int open_log(const char *path){
	time_t now, seconds_to_next_day;
	struct tm tm_now;

	/* get the current time */
	time(&now);
	localtime_r(&now, &tm_now);

	seconds_to_next_day = 86400 - 3600 * tm_now.tm_hour - 
		60 * tm_now.tm_min - tm_now.tm_sec;
	/* 
	 * an extra 1min to make sure when time() exceeds next_day_mark,
	 * it's really next day
	 */
	next_day_mark = now + seconds_to_next_day + 60;

	/* generate the path yyyymmdd.log according to the time */
	snprintf(logpath, FILENAME_MAX, "%s/%04d%02d%02d.log.gz", path, 
			tm_now.tm_year+1900, tm_now.tm_mon+1, tm_now.tm_mday);

	/* if the log exits, decopress it first */
	if(access(logpath, F_OK) == 0)
		decompress(logpath);
	logpath[strlen(logpath)-3] = '\0';

	log_fp = fopen(logpath, "a");
	if(log_fp == NULL)
		return STATUS_LOG_ERR;
	
	log_size = 0;
	write_log(INFO, "Open the logfile by PID: %d", getpid());
	return STATUS_OK;
}

/**
 * @brief write the log, the log file will be flushed every FLUSH_LOG_NUM records
 *
 * @param type log type
 * @param format log content
 * @param ...
 *
 * @return STATUS_OK
 */
int write_log(enum log_type type, const char *format, ...){
	char log_entry[MAXBUFSIZE];
	va_list arg_ptr;
	int len;
	time_t now;

	va_start(arg_ptr, format);
	len = vsprintf(log_entry, format, arg_ptr);
	va_end(arg_ptr);

	time(&now);
	fprintf(log_fp, "[%s] %s\t%s\n\n", log_names[type], ctime(&now),
			log_entry);
	
	if(++log_size % FLUSH_LOG_NUM == 0){
		log_size = 0;
		fflush(log_fp);
	}

	if(now > next_day_mark){
		/* close and compress the log */
		close_log();
		*strrchr(logpath, '/') = '\0';
		/* open the new log */
		open_log(logpath);
	}

	return STATUS_OK;
}

/**
 * @brief close the log and compress it
 *
 * @return STATUS_OK
 */
int close_log(){
	write_log(INFO, "Close the log by PID: %d", getpid());
	fclose(log_fp);
	compress(logpath);
	return STATUS_OK;
}

/**
 * @brief compress the log
 *
 * @param path log path
 *
 * @return 0 on success and other on failure
 */
static int compress(const char *path){
	char cmd[FILENAME_MAX + 20];
	if(access("/bin/gzip", X_OK) || access(path, F_OK) )
		return 1;

	snprintf(cmd, sizeof(cmd), "/bin/gzip -qf %s", path);

	return wrap_system(cmd);
}

/**
 * @brief decompress the log
 *
 * @param path log path
 *
 * @return 0 on success and other on failure
 */
static int decompress(const char *path){
	char cmd[FILENAME_MAX + 20];
	if(access("/bin/gzip", X_OK) || access(path, F_OK) )
		return 1;

	snprintf(cmd, sizeof(cmd), "/bin/gzip -d %s", path);

	return wrap_system(cmd);
}
