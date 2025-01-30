/* (c) 1996-2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */

/* Unix /etc/passwd Authentication Database Interface
 *
 * This module supports use of an old-style /etc/passwd with passwords in it
 * as an authentication database.
 *
 * It is actually a pretty complete lack of a module.  We can't edit the
 * file and since the auth database is also the ident database, the ident_*.c
 * module provides all the tools we need to access it.  So this file contains
 * just a shrug and a smile.
 */

char *showopt_auth_module= "auth_passwd";
