#ifndef	RESH_H__
#define	RESH_H__

struct rcommand {
	char *argv[16 + 1];
	int argc;
};

#define	NELEM(A)	(sizeof(A) / sizeof((A)[0]))

char *resh_read(const char *prompt);
int openr(const char *filename);

#endif	/* !RESH_H__ */
