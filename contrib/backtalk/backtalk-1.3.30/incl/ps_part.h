/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

void closepart(void);
int openpart(int snoop);
void last_read(int item, int *resp_no, int *forgotten, time_t *date);
void update_item(int item, int resp, time_t date, int rewind);
void reset_item(int item);
void lost_items(int first,int last);
void pf_rfav(int item);
void set_conf_alias(char *newname);
time_t partdate(void);
int fav_sel(char *isel, int flag, char **i1, char **i2, char **i3);
