#ifdef IISPELL_PATH
# define SPELLCHECK 1
# define SPELL_PATH IISPELL_PATH
#endif

#ifdef ASPELL_PATH
# define SPELLCHECK 1
# define SPELL_PATH ASPELL_PATH
#endif

#ifndef SPELLCHECK
# define SPELLCHECK 0
#endif

void func_spellcheck(void);
int spellcheck_step(char *text, int *i, int mode, int type);
void close_spellcheck(void);
