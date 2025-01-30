/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

int hide_resp(int item, int resp, int hide);
int erase_resp(int item, int resp);
int item_mode(int item, int flag);
int kill_item(int item);
int link_item(int that_item, char *that_conf, int *this_item);
int retitle_item(char *newtitle);
int edit_resp(int repl_text, char *delattach, char *addattach);

#define IM_FREEZE 0
#define IM_THAW 1
#define IM_RETIRE 2
#define IM_UNRETIRE 3
