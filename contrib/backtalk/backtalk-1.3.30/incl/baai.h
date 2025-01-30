/* Copyright 2003, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

void baai_set(char *fname, int link, char *type, off_t size, char *user,
	uid_t uid, char *conf, int access, time_t date, char *desc);

int baai_get(char *fname, int *link, char **type, off_t *size, char **user,
	uid_t *uid, char **conf, int *access, time_t *date, char **desc);

int baai_del(char *fname);

int baai_link(char *fname);

int baai_walk(int next, char **fname, int *link, char **type, off_t *size,
	char **user, uid_t *uid, char **conf, int *access, time_t *date,
	char **desc);
