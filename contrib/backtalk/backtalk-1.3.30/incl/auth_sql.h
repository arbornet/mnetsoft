/* Copyright 2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */


/* AUTHENTICATION MODULE INTERFACE - SQL Database Version
 *
 * This header describes the interface to an authentication module.  Each
 * authentication module implements various operations on the authentication
 * database, but not all modules implement all operations.  This defines
 * which have been implemented for a particular authentication module.
 *
 * Note that not all of these entry points are needed in all configurations.
 * The defines for the unneeded ones will be undefined later.  This should
 * define everything the module can do.
 */

/* FUNCTION: auth_getpass(login)
 *
 *    Returns the encrpyted password for the user, or NULL if the user is not
 *    found.  Value is returned in static storage.
 *
 *    Optional.  Without it we can't check passwords or status, unless the
 *    auth file is shared with the ident file, in which case we'll use
 *    ident_getpwnam instead.
 */

#define HAVEAUTH_GETPASS

/*  FUNCTION: auth_checkpass(login,passwd)
 *
 *    Given a login and a plain text password, return true if they are valid.
 *
 *    This is never used unless ID_SESSION is defined.  If we have ID_SESSION
 *    we must have either auth_checkpass() or auth_getpass(). If checkpass()
 *    is not implemented, we compare the result of getpass() with the result
 *    of running crypt() on the password.
 */

#undef HAVEAUTH_CHECKPASS		/* Default action is fine */

/*  FUNCTION: auth_walk(flag)
 *
 *     If flag is 0, return the login name for the "first" user in
 *     the database.  Otherwise, return the "next" user in the database.
 *     returned name is in static storage.
 *
 *     Required only if neither ident_walk() nor ident_walkpw() is defined.
 */

#define HAVEAUTH_WALK

/*  FUNCTION: auth_seek(login)
 *
 *     Arrange things so that the next call to auth_walk(1) will return the
 *     user after the given user.
 *
 *     Used only if neither ident_walk() nor ident_walkpw() is defined, and
 *     optional even then.  If it is missing, we use auth_walk() to go
 *     sequentially through the database until the desired login is found.
 */

#define HAVEAUTH_SEEK

/*  FUNCTION:  auth_newpass(login,encpw)
 *
 *    Change a user's password, given his login and encrypted password.
 *
 *    Optional.  If not implemented, Backtalk cannot change passwords.
 */

#define HAVEAUTH_NEWPASS

/*  FUNCTION:  auth_add(login,password,status,uid,gid,fname,dir)
 *
 *    Add a user to the authentication database.  The uid,gid,fname and dir
 *    arguments should be ignored the auth and ident database is not shared.
 *    Otherwise they should be saved too.
 *
 *    Optional.  If not implemented, Backtalk cannot create users.
 */

#define HAVEAUTH_ADD

/*  FUNCTION: auth_del(login)
 *
 *    Delete a user from the authentication database.  Return non-zero if
 *    the user does not exist, 0 on success.
 *
 *    Optional.  If not implemented, Backtalk cannot delete users.
 */

#define HAVEAUTH_DEL
