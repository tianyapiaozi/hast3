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
#ifndef _KEYFILE_H_
#define _KEYFILE_H_

#include <glib.h>

typedef GKeyFile Keyfile;

/* keyfile.h -- public functions prototypes */
int initKeyfile(Keyfile **keyfile, const char *config);
int saveKeyfile(Keyfile *keyfile, const char *config);
int destroyKeyfile(Keyfile *keyfile);
int getIntValue(Keyfile *keyfile, const char *sec, const char *key, int *value);
int getStrValue(Keyfile *keyfile, const char *sec, const char *key, char *value);
int getFloatValue(Keyfile *keyfile, const char *sec, const char *key, double *value);
int setIntValue(Keyfile *keyfile, const char *sec, const char *key, int value);
int setStrValue(Keyfile *keyfile, const char *sec, const char *key, const char *value);
int setFloatValue(Keyfile *keyfile, const char *sec, const char *key, double value);
int getStrValueList(Keyfile *keyfile, const char *sec, const char *key, char ***value, int size);
int setStrValueList(Keyfile *keyfile, const char *sec, const char *key, const char * const value[], int size);

#endif /* _KEYFILE_H_ */
