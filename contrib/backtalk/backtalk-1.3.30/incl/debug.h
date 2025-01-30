void dump_env(FILE *fp, int quote);
char *mgets(FILE *fp);
void load_repeat(char *fname);
extern char **old_env;
#ifdef CLEANUP
void free_repeat(void);
#endif /* CLEANUP */
