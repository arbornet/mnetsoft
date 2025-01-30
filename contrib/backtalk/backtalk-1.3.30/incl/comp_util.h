/* Copyright 1996, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

void init_dict(void);
Token *add_const(char *label);
int def_constant(Token *var, Token *val, int sysonly);
void undef_constant(Token *var);
void loadconstants(void);
void dump_dict(HashTable *dict, FILE *fp);
Token *get_token(FILE *fp);

int add_file_name(char *fname);
char *find_file_name(int index);
void write_file_list(int fd);
void empty_file_list(void);

extern HashTable constdict;
