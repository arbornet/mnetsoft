/*
 * Copyright (C) 2010  Cyberspace Communications, Inc.
 * All Rights Reserved
 *
 * $Id: tcl.c 1006 2010-12-08 02:38:52Z cross $
 *
 * Interface to a Tcl interpreter to get information from a Tcl variable.
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <tcl.h>

static Tcl_Interp *tcl = NULL;
const char *CONFFILE = PNUHOME "/lib/nuconfig.tcl";

static void
tclend(void)
{
	if (tcl != NULL) {
		Tcl_DeleteInterp(tcl);
		tcl = NULL;
	}
}

static void
tclerror(const char *prefix)
{
	int len;
	Tcl_Obj *result_object;
	char *result;

	fprintf(stderr, "%s", prefix);
	if (tcl == NULL) {
		fprintf(stderr, "\n");
		return;
	}
	result_object = Tcl_GetObjResult(tcl);
	if (result_object == NULL) {
		fprintf(stderr, "\n");
		return;
	}
	len = 0;
	result = Tcl_GetStringFromObj(result_object, &len);
	if (result == NULL || len <= 0) {
		fprintf(stderr, "\n");
		return;
	}

	fprintf(stderr, ": %.*s\n", len, result);
}

static void
tclinit(void)
{
	if (tcl != NULL)
		return;

	tcl = Tcl_CreateInterp();
	if (tcl == NULL)
		tclerror("Error creating Tcl interpreter");
	if (Tcl_Init(tcl) != TCL_OK)
		tclerror("Error in Tcl_Init");
	atexit(tclend);
	if (Tcl_EvalFile(tcl, CONFFILE) != TCL_OK)
		tclerror("Error in Tcl_EvalFile");
}

char *
savetclstring(Tcl_Obj *string)
{
	char *p, *s;
	int len;

	len = 0;
	p = Tcl_GetStringFromObj(string, &len);
	if (p == NULL || len <= 0) {
		tclerror("Tcl_GetStringFromObj failed");
		return(strdup(""));
	}
	s = malloc(len + 1);
	if (s == NULL)
		return(strdup(""));
	memmove(s, p, len);
	s[len] = '\0';

	return(s);
}

Tcl_Obj *
getarrayelement(const char *varname)
{
	Tcl_Obj *config, *element, *value;

	config = Tcl_NewStringObj("config", strlen("config"));
	if (config == NULL) {
		tclerror("Tcl_NewStringObj(config) failed");
		return(NULL);
	}
	element = Tcl_NewStringObj(varname, strlen(varname));
	if (element == NULL) {
		tclerror("Tcl_NewStringObj(varname) failed");
		return(NULL);
	}
	value = Tcl_ObjGetVar2(tcl, config, element, 0);
	if (value == NULL) {
		tclerror("Tcl_ObjGetVar2 failed");
		return(NULL);
	}

	return(value);
}

char *
getvalue(const char *varname)
{
	Tcl_Obj *value;

	tclinit();
	value = getarrayelement(varname);
	if (value == NULL)
		return(strdup(""));

	return(savetclstring(value));
}

char **
getlist(const char *varname)
{
	char **arr;
	Tcl_Obj *list, **objects;
	int i, alen;

	tclinit();
	arr = NULL;
	alen = 0;

	list = getarrayelement(varname);
	if (list == NULL)
		return NULL;
	if (Tcl_ListObjGetElements(tcl, list, &alen, &objects) != TCL_OK)
		return NULL;
	arr = malloc((alen + 1) * sizeof(char *));
	if (arr == NULL)
		return NULL;
	memset(arr, 0, (alen + 1) * sizeof(char *));
	arr[alen] = NULL;
	for (i = 0; i < alen; i++) {
		arr[i] = savetclstring(objects[i]);
	}

	return(arr);
}

/*
#include <stdio.h>

int
main(void)
{
	char	*val, **arr, **arrp;

	val = getvalue("uidrange");
	arr = getlist("userdev");
	if (val != NULL) {
		printf("uidrange = %s\n", val);
		free(val);
	}
	if (arr != NULL) {
		printf("userdevs:\n");
		for (arrp = arr; *arrp; arrp++) {
			printf("0x%p: %s\n", *arrp, *arrp);
			free(*arrp);
		}
		free(arr);
	}

	return(EXIT_SUCCESS);
}
*/
