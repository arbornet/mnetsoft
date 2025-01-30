/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

void opensum(char *dir);
void closesum(void);
void seek_item(int item_no);
int last_no(void);
int get_sum(int item, int *resp_no, int *flags,
	time_t *item_date, time_t *resp_date);
int put_sum(int item, int resp_no, int sflags, int rflags,
	time_t item_date, time_t resp_date);
int next_item(int *item_no, int *resp_no, int *flags,
		time_t *item_date, time_t *resp_date);
int prev_item(int *item_no, int *resp_no, int *flags,
		time_t *item_date, time_t *resp_date);
void link_sum(char *src_dir, int src_item, char *dst_dir, int dst_item);

#ifndef SF_RETIRED

/* Sum File Flags */
#define SF_RETIRED 0x0002               /* Retired item */
#define SF_DUNNO   0x000c
#define SF_ACTIVE  0x0030               /* Active item */
#define SF_FROZEN  0x0040               /* Frozen item */
#define SF_PARTY   0x0080               /* Party item */
#define SF_LINKED  0x0100               /* Linked item */

#endif
