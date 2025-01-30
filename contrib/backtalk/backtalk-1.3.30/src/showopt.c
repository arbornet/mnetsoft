/* Copyright 2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

#include "common.h"
#include "showopt.h"
#include "version.h"
#include "authident.h"
#include "hashfile.h"
#include "nextuid.h"
#include "group.h"
#include "sess.h"
#include "sql.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


/* SHOW_OPTIONS - Generate a printable string describing the system
 * configuration.  This is rather large for a one-line procedure.
 * Returned string is in dynamic memory.
 */

char *show_options(int html)
{
    char *endline= (html ? "<BR>" : "");
    char *endpara= (html ? "<P>" : "\n");
    char bf[2*BFSZ + 1];

    snprintf(bf, 2*BFSZ,
	"Backtalk Version %d.%d.%d"VERSION_NOTE"%s\n"

	"Authentication Database Module: %s%s\n"
	"Identity Database Module: %s%s\n"
	"Next UID Database Module: %s%s\n"
	"Group Database Module: %s%s\n"
	"Session Database Module: %s%s\n"

#if EDITUSER
	"User Editing: Enabled%s\n"
#else
	"User Editing: Disabled%s\n"
#endif

#ifdef DYNALOAD
	"Dynamic Loading: Enabled (dir="DYNALIB_DIR")%s\n"
#else
	"Dynamic Loading: Disabled%s\n"
#endif

#if defined(YAPP_COMPAT)
#ifdef YAPP3_COMPAT
	"Compatibility: Yapp 3.0"
#else
	"Compatibility: Yapp 2.3"
#endif
#ifdef PARTUTIL
	" (partutil="PARTUTIL")"
#endif
	"%s\n"
#else
#ifdef PICOSPAN_COMPAT
	"Compatibility: Picospan"
#ifdef PARTUTIL
	" (partutil="PARTUTIL")"
#endif
	"%s\n"
#else
	"Compatibility: None%s\n"
#endif
#endif

#ifdef LOCK_FCNTL
	"Locking: fcntl()%s\n"
#endif
#ifdef LOCK_FLOCK
	"Locking: flock()%s\n"
#endif
#ifdef LOCK_LOCKF
	"Locking: lockf()%s\n"
#endif
#ifdef LOCK_LOCKF
	"Locking: temp file (ugh!)%s\n"
#endif

        "Max Login Length: %d%s\n"
        "BBS Directory: "BBS_DIR"%s\n"
        "Script Directory: "SCRIPT_DIR"%s\n"
        "User Directory: "USER_DIR" (levels=%d)%s\n"
        "Config File: "CONFIG_FILE"%s\n"
        "Compiler Path: "BT_COMPILER"%s\n"
        "Account Creation Log: "ACCT_LOG_FILE"%s\n"
        "Crash Log: "ERROR_FILE"%s\n"
        "Transaction Log: "LOG_FILE"%s\n"
#ifdef LASTLOG_FILE
        "Last Login Log: "LASTLOG_FILE"%s\n"
#else
        "Last Login Log: none%s\n"
#endif

#ifdef TRAP_CRASH
	"Trap Crash: Enabled%s\n"
#else
	"Trap Crash: Disabled%s\n"
#endif

	"Hash File Library: %s%s\n"
	"SQL Server Interface: %s%s\n"

#ifdef IISPELL_PATH
	"Spell Checker: International Ispell (path="IISPELL_PATH")%s\n"
#else
	"Spell Checker: none%s\n"
#endif

	"Owner UID: %d%s\n"
	"HttpD UID: %d%s\n"

	,

	/* Version */		VERSION_A, VERSION_B, VERSION_C, endpara,
	/* Auth Module */	showopt_auth_module, endline,
	/* Ident Module */	showopt_ident_module, endline,
	/* Nextuid Module */	showopt_nextuid_module, endline,
	/* Group Module */	showopt_group_module, endline,
	/* Session Module */	showopt_session_module, endpara,
	/* User Editing */	endline,
	/* Dynamic Loading */	endline,
	/* Compatibility */	endline,
	/* Locking */		endline,
	/* Max Login Len */	MAX_LOGIN_LEN, endpara,
	/* BBS Dir */		endline,
	/* Script Dir */	endline,
	/* User Dir */		USER_DIR_LEVEL, endline,
	/* Config File */	endline,
	/* Compiler Path */	endline,
	/* Account Create Log */endline,
	/* Crash Log */		endline,
	/* Transaction Log */	endline,
	/* Last Login Log */	endline,
	/* Trap Crash */	endpara,
	/* Hash File Lib */	showopt_hash_lib, endline,
	/* SQL Server */	showopt_sql_module, endline,
	/* Spell Check */	endpara,
	/* Owner UID */		CFADM_UID, endline,
	/* HttpD UID */		SERVER_UID, endline
	);

    return strdup(bf);
}
