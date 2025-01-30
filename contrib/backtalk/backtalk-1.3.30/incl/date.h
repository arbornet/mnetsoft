/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include <time.h>

void addtm(struct tm *tm, int secs, int mins, int hours,
                          int days, int mons, int years);
void subtm(struct tm *tm, int secs, int mins, int hours,
                          int days, int mons, int years);
time_t strtime(char *string, int future, int start, char *tz, struct tm *tm);

char *ctimez(time_t *clock, char *tz);
struct tm *localtimez(time_t *clock, char *tz);
time_t mktimez(struct tm *tm, char *tz);
char *asctimez(struct tm *tm, char *tz);

int tz_offset(struct tm *tm);

char *http_time(time_t time, char *bf);
char *mime_time(time_t time, char *bf);
