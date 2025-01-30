/* Copyright 2003, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

FILE *open_temp_attach(char **tmpfile);
char *temp_attach_handle(char *tmpname, char *origname, char *ctype);
char *make_attach(char *tmphand, char *desc, char *cf, int in, int rn,
	        int access);
void func_edit_attach(void);
void func_get_attach(void);
void baaf_file(char *hand);
