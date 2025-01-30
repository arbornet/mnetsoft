#include <time.h>

/* Only update lastlog files if we are using a Backtalk lastlog file */

#ifdef LASTLOG_FILE
void update_lastlog(int uid);
#else
#define update_lastlog(u)
#endif

/* But we can read any kind of lastlog file */

#if defined(LASTLOG_FILE) || defined(UNIX_LASTLOG)
time_t get_lastlog(int uid);
#else
#define get_lastlog(u) 0
#endif
