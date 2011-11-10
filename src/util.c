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
 * @file util.c
 * @brief 
 * @author Li Jiliang<tjulijiliang@gmail.com
 * @version 1.0
 * @date 2011-11-09
 */
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/wait.h>

#include "hast3.h"
#include "log.h"

/**
 * @brief wrap of the system(3)
 *
 * @param cmd command
 *
 * @return -1 on failure, real exit code on success
 */
int wrap_system(const char* cmd){
	int status;

	errno = 0;
	status = system(cmd);
	if(status == -1){
		write_log(ERROR, "Failed to execute [%s] with error message:%s",
				cmd, strerror(errno));
		return -1;
	}

	status = WEXITSTATUS(status);
	if(debug_level > 2)
		write_log(DEBUG, "Exit code of [%s] is %d, with error:%s",
				cmd, status, strerror(errno));

	return status;
}
