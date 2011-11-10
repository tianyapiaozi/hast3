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
 * @file keyfile.c
 * @brief 
 * @author Li Jiliang<tjulijiliang@gmail.com
 * @version 1.0
 * @date 2011-11-09
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "keyfile.h"

#define CMD_LEN		2048

/**
 * @brief init the Keyfile struct using glib library
 *
 * @param keyfile pointer to Keyfile pointer
 * @param config config file path
 *
 * @return 0
 */
int
initKeyfile(Keyfile **keyfile, const char *config)
{
	GError	*err = NULL;
	GKeyFile *__keyfile = g_key_file_new();

	g_key_file_set_list_separator(__keyfile, ';');

	if (!g_key_file_load_from_file(__keyfile, config, 0, &err)) {
		fprintf(stderr, "Cannot load config file: %s\n", config);
		g_error_free(err);
		g_key_file_free(__keyfile);
		return 1;
	}

	*keyfile = (Keyfile*)__keyfile;
	return 0;
}

/**
 * @brief Get the integer value with the given key from the given section in the keyfile
 *
 * @param keyfile Keyfile pointer
 * @param sec section name
 * @param key key name
 * @param value pointer to the result
 *
 * @return 0 on success and 1 on failure
 */
int 
getIntValue(Keyfile *keyfile, const char *sec, const char *key, int *value)
{
	GError *err = NULL;
 
	*value = g_key_file_get_integer(keyfile, sec, key, &err);
	if (err) {
		g_error_free(err);			
		return 1;
	}
	
	return 0;
}

/**
 * @brief set the integer value with the given key and section into the keyfile
 *
 * @param keyfile Keyfile pointer
 * @param sec section name
 * @param key key name
 * @param value value
 *
 * @return 0
 */
int 
setIntValue(Keyfile *keyfile, const char *sec, const char *key, int value)
{
	g_key_file_set_integer(keyfile, sec, key, value);
	
	return 0;
}

/**
 * @brief get the string value with the given key
 *
 * @param keyfile Keyfile pointer
 * @param sec section name
 * @param key key name
 * @param value pointer to the result
 *
 * @return 0 on success and 1 on failure
 */
int 
getStrValue(Keyfile *keyfile, const char *sec, const char *key, char *value)
{
	GError *err = NULL;
	char *str = g_key_file_get_string(keyfile, sec, key, &err);

	if (err) {
		g_error_free(err);
		return 1;
	}

	strcpy(value, str);
	g_free(str);
	
	return 0;

}


/**
 * @brief set the string value with the given key
 *
 * @param keyfile Keyfile pointer
 * @param sec section name
 * @param key key name
 * @param value value
 *
 * @return 0
 */
int 
setStrValue(Keyfile *keyfile, const char *sec, const char *key, 
		const char *value)
{
	g_key_file_set_string(keyfile, sec, key, value);
	
	return 0;

}

/**
 * @brief get the string list with the given key
 *
 * @param keyfile Keyfile pointer
 * @param sec section name
 * @param key key name
 * @param value where the result should be stored
 * @param size size of the list
 *
 * @return 
 */
int 
getStrValueList(Keyfile *keyfile, const char *sec, const char *key, 
		char ***value, int size)
{
	GError *err = NULL;
	int i;
	gsize length;
	char **str = g_key_file_get_string_list(keyfile, sec, key, &length, 
			&err);

	if (err) {
		g_error_free(err);
		return 1;
	}
	if(length != size){
		g_strfreev(str);
		return 1;
	}
	*value = (char **)malloc(sizeof(char *) * (length+1));
	for(i = 0; i < size; i++)
		(*value)[i] = strdup(str[i]);
	(*value)[size] = NULL;

	g_strfreev(str);
	return 0;

}

/**
 * @brief set the string list with the given key
 *
 * @param keyfile Keyfile pointer
 * @param sec section name
 * @param key key name
 * @param value[] value
 * @param size size of the list
 *
 * @return 0
 */
int setStrValueList(Keyfile *keyfile, const char *sec, const char *key, 
		const char * const value[], int size)
{
	g_key_file_set_string_list(keyfile, sec, key, value, size);
	
	return 0;

}


/**
 * @brief get the double value with the given key
 *
 * @param keyfile Keyfile pointer
 * @param sec section name
 * @param key key name
 * @param value where the result should be stored
 *
 * @return 0
 */
int 
getFloatValue(Keyfile *keyfile, const char *sec, const char *key, 
		double *value)
{
	GError *err = NULL;
	*value = g_key_file_get_double(keyfile, sec, key, &err);

	if (err) {
		g_error_free(err);
		return 1;
	}
	
	return 0;
}

/**
 * @brief set the double value with given key
 *
 * @param keyfile Keyfile pointer
 * @param sec section name
 * @param key key name
 * @param value value
 *
 * @return 0
 */
int 
setFloatValue(Keyfile *keyfile, const char *sec, const char *key, 
		double value)
{
	g_key_file_set_double(keyfile, sec, key, value);
	
	return 0;
}

/**
 * @brief save the keyfile key-value pairs into the configure file
 *
 * @param keyfile Keyfile pointer
 * @param config configure file path
 *
 * @return 0 on success and 1 on failure
 */
int saveKeyfile(Keyfile *keyfile, const char *config)
{
	char cmd[CMD_LEN], *content;
	FILE *file = NULL;
	GError *err = NULL;
	gsize length;
	
	content = g_key_file_to_data(keyfile, &length, &err);

	if (err) {	
		g_error_free(err);	
		g_free(content);

		return 1;
	}
	
	//backup the old config file:
	sprintf(cmd, "mv %s %s.bak", config, config);
	system(cmd);

	if ( (file = fopen(config, "a+")) == NULL ) {

		g_free(content);
		return 1;
	}

	fwrite(content, sizeof(gchar), length, file);

	g_free(content);

	fclose(file);

	return 0;
}

/**
 * @brief release the keyfile struct 
 *
 * @param keyfile Keyfile pointer
 *
 * @return 0
 */
int destroyKeyfile(Keyfile *keyfile)
{
	g_key_file_free((GKeyFile*)keyfile);

	return 0;
}

