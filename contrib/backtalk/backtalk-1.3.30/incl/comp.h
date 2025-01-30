/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Defines for Backtalk compiler */

/* Header for compiled files */

typedef struct {
	char magic[4];		/* Magic number */
	char version[4];	/* Compiler Version number */
	time_t confdate;	/* mtime of config.bt used */
	} CompHead;

#define MAGIC "(-:"		/* smiley face is our magic number */

extern int curr_line;		/* current line number */
