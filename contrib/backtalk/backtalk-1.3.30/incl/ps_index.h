/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

void item_index_name(char *bf, int bfsz, char *confdir, int itemnumber);
void remove_index(char *confdir, int itemnumber);
void link_index(char *that_conf, int that_item, char *this_conf, int this_item);
void close_item_index(void);
void open_item_index(char *confdir, int itemnumber);
void index_new_resp(char *confdir, int inum, int rnum, size_t roffset);
int seek_resp(FILE *item_fp, int resp);
void rebuild_index(FILE *item_fp, char *confdir, int itemnumber);
