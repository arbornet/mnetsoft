/* formvars.h
 * ------------------------------------------
 * Copyright (c) 1997 by John H. Remmers
 * ------------------------------------------
 * $Id: formvars.h 1628 2017-09-20 00:41:13Z cross $
 * ------------------------------------------
 * Header file for modules that use formvars.c
 */

char **getcgivars(void);        /* build table of (name,value) pairs */
char *cgival(char *name);       /* get value for specified name */
