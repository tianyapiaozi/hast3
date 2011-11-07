/* Copyright (C) 
 * 2010 - Amal Cao
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "keyfile.h"

#define CMD_LEN		2048

/* 
 * ===  FUNCTION  =======================================================
 *         Name:  initKeyfile
 *  Description:  Init the keyfile sruct using glib library.
 * ======================================================================
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


/* 
 * ===  FUNCTION  =======================================================
 *         Name:  getIntValue
 *  Description:  Get the integer value with the given key from the given 
 *                section in the keyfile.
 * ======================================================================
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


/* 
 * ===  FUNCTION  =======================================================
 *         Name:  setIntValue
 *  Description:  Set the integer value with the given key and section 
 *                into the keyfile.
 * ======================================================================
 */
int 
setIntValue(Keyfile *keyfile, const char *sec, const char *key, int value)
{
	g_key_file_set_integer(keyfile, sec, key, value);
	
	return 0;
}


/* 
 * ===  FUNCTION  =======================================================
 *         Name:  getStrValue
 *  Description:  Get the string value with the given key...
 * ======================================================================
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


/* 
 * ===  FUNCTION  =======================================================
 *         Name:  setStrValue
 *  Description:  Set the string value with the given key... 
 * ======================================================================
 */
int 
setStrValue(Keyfile *keyfile, const char *sec, const char *key, 
		const char *value)
{
	g_key_file_set_string(keyfile, sec, key, value);
	
	return 0;

}
/* 
 * ===  FUNCTION  =======================================================
 *         Name:  getStrValueList
 *  Description:  Get the string list with the given key...
 * ======================================================================
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


/* 
 * ===  FUNCTION  =======================================================
 *         Name:  setStrValueList
 *  Description:  Set the string list with the given key... 
 * ======================================================================
 */
int setStrValueList(Keyfile *keyfile, const char *sec, const char *key, 
		const char * const value[], int size)
{
	g_key_file_set_string_list(keyfile, sec, key, value, size);
	
	return 0;

}


/* 
 * ===  FUNCTION  =======================================================
 *         Name:  getFloatValue
 *  Description:  Get the double value with the given key ...
 * ======================================================================
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

/* 
 * ===  FUNCTION  =======================================================
 *         Name:  setFloatValue
 *  Description:  Set the double value with given key ...
 * ======================================================================
 */
int 
setFloatValue(Keyfile *keyfile, const char *sec, const char *key, 
		double value)
{
	g_key_file_set_double(keyfile, sec, key, value);
	
	return 0;
}

/* 
 * ===  FUNCTION  =======================================================
 *         Name:  saveKeyfile
 *  Description:  Save the keyfile key-value pairs into the configure file.
 * ======================================================================
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


/* 
 * ===  FUNCTION  =======================================================
 *         Name:  destroyKeyfile
 *  Description:  Release the keyfile struct...
 * ======================================================================
 */
int destroyKeyfile(Keyfile *keyfile)
{
	g_key_file_free((GKeyFile*)keyfile);

	return 0;
}

