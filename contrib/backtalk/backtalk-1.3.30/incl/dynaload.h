/* Copyright 2000, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* If editting these, also edit the table in dynaload.c */

#define LIB_USRADM	0
#define LIB_USRADM_NAME "lib_usradm"

#define LIB_GRPADM	1
#define LIB_GRPADM_NAME "lib_grpadm"

#define LIB_CNFADM	2
#define LIB_CNFADM_NAME "lib_cnfadm"

#define N_LIB		3

void (*dynaload(Token *))(void);
