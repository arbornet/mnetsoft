/* (c) 1996-2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* GENERIC INTERFACE TO GROUP DATABASE */

char *groupname(gid_t gid);
int groupid(char *gname);
int ingroupno(gid_t gid, gid_t my_gid, char *my_login);
int ingroupname(char *groupname, gid_t my_gid, char *my_login);
int ingrouplist(char **grouplist, gid_t my_gid, char *my_login);
void for_groups(void (*func)(char *));

char *showopt_group_module;

#if !NOPWEDIT
int newgroup(char *gname);
int newgroupmem(char *gname, char *mem);
int delgroup(char *gname);
int delgroupmem(char *gname, char *member);
#endif

/* If we have compiled-in groups, some functions turn into macros */

#ifdef USER_GID
# define user_group_id() USER_GID
# ifdef USER_GROUP
#  define user_group_name() USER_GROUP
# else
   char *user_group_name(void);
# endif
#else
# ifdef USER_GROUP
   int user_group_id(void);
#  define user_group_name() USER_GROUP
# else
#  define user_group_id() -1
#  define user_group_name() NULL
# endif
#endif

#ifdef CFADM_GID
# define cfadm_group_id() CFADM_GID
# ifdef CFADM_GROUP
#  define cfadm_group_name() CFADM_GROUP
# else
   char *cfadm_group_name(void);
# endif
#else
# ifdef CFADM_GROUP
   int cfadm_group_id(void);
#  define cfadm_group_name() CFADM_GROUP
# else
#  define cfadm_group_id() -1
#  define cfadm_group_name() NULL
# endif
#endif

#ifdef GRADM_GID
# define gradm_group_id() GRADM_GID
# ifdef GRADM_GROUP
#  define gradm_group_name() GRADM_GROUP
# else
   char *gradm_group_name(void);
# endif
#else
# ifdef GRADM_GROUP
   int gradm_group_id(void);
#  define gradm_group_name() GRADM_GROUP
# else
#  define gradm_group_id() -1
#  define gradm_group_name() NULL
# endif
#endif
