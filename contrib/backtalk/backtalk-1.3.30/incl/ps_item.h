/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

int openitem(int excl);
void closeitem(void);
int itemhead(void);
int resphead(int resp);
char *resptext(int form);
time_t atoth(char *str);
void item_powers(void);
void resp_powers(void);
