/*
 * $Id: libgrex.h 1006 2010-12-08 02:38:52Z cross $
 */

#ifndef	LIBGREX_H__
#define	LIBGREX_H__

enum
{
	PASSRESLEN = 32
};

#define	STREQ(a, b)	(((a)[0] == (b)[0]) && (strcmp((a), (b)) == 0))
#define	GREXHASHREALM	"grex.cyberspace.org"
#ifdef	MISSING_STRLCPY
#define	strlcpy(a, b, l)	strcpy(a, b)
#endif

extern	char *libgrex_program_name__;

int	cmpstring(const void *, const void *);
void	daemonize(char *, char *);
char	*grexhash_r(char *, char *, char *, size_t);
char	*progname(char *);
int	isshellvalid(char *);
int	isshellok(int);
void	background(void);
void	fatal(char *, ...);
void	warning(char *, ...);

/*
 * ADTs for thread objects.
 */
typedef	void *Thread;
typedef	void *RWLock;

void	threadspawn(Thread *, void *(*)(void *), void *);
void	wrlock(RWLock *);
void	rdlock(RWLock *);
void	unlock(RWLock *);

#endif	/* !LIBGREX_H__ */
