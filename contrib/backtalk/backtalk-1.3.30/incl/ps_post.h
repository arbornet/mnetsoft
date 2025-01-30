/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

void write_text(FILE *fp, uid_t uid, char *id, char *alias, int style,
    char *text, char *alt_text, time_t postdate, time_t editdate,
    int parent, char *attach, size_t *offset, size_t *size);
int post_resp(void);
void post_item(void);
