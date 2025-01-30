/* Copyright 2001, Jan D. Wolter and Steven R. Weiss, All Rights Reserved. */


/* IDENTITY MODULE INTERFACE - SQL Database Version
 *
 * This header describes the interface to an identity database module.  Each
 * identity module implements various operations on the identity database,
 * but not all modules implement all operations.  This defines which have
 * been implemented for a particular identity module.
 *
 * Note that not all of these entry points are needed in all configurations.
 * The defines for the unneeded ones will be undefined later.  This should
 * define everything the module can do.
 */

/* CONSTANT:  IDENT_STOREDIR
 *
 *     If defined, our ident database includes a stored home directory path for
 *     each user.  If false, home directory paths should be computed from the
 *     login name.  If storedir is true, ident_getpwnam(), ident_getpwuid(), and
 *     ident_walkpw() should return the stored value.  If not, they should
 *     generally return pw_dir=NULL.
 */

#undef IDENT_STOREDIR

/* FUNCTION: ident_getpwnam(login)
 *
 *     Given a login name, return a 'struct passwd' with at least pw_uid,
 *     pw_gid, and pw_gecos (fullname) fields set.  pw_name, pw_passwd, pw_dir,
 *     and pw_shell may be either set to something, or NULL.  The returned
 *     pointer and all string pointers in the returned structure should point
 *     to static storage.  Return NULL if no user with that name exists.
 *
 *     Always required.
 */

#define HAVEIDENT_GETPWNAM

/* FUNCTION: ident_getpwuid(uid)
 *
 *     Same as getidnam, but finds the user with the given UID number
 *     instead of the given login and the returned passwd structure must
 *     have both pw_name and pw_uid defined.
 *
 *     Required only if we enable direct execution.
 */

#define HAVEIDENT_GETPWUID

/* FUNCTION: ident_walkpw(flag)
 *
 *     If flag is 0, return the passwd structure for the "first" user in
 *     the database.  Otherwise, return the "next" user in the database.
 *     Returned passwd structure should have pw_name, pw_uid, pw_gid,
 *     and pw_gecos.
 *
 *     Either ident_walkpw, ident_walk or auth_walk is required.  If this is
 *     not defined we do ident_getpwnam(ident_walk(flag)) or 
 *     ident_getpwnam(auth_walk(flag)), so unless you can do better you don't
 *     need to implement this.
 */

#define HAVEIDENT_WALKPW

/* FUNCTION: ident_walk(flag)
 *
 *     If flag is 0, return the login name for the "first" user in
 *     the database.  Otherwise, return the "next" user in the database.
 *     returned name is in static storage.
 *
 *     Either ident_walkpw or ident_walk is required.  If ident_walk() is not
 *     defined we do auth_walk or ident_walkpw(flag)->pw_name, so this need
 *     only be provided if we can do something faster.
 */

#define HAVEIDENT_WALK

/* FUNCTION: ident_seek(login)
 *
 *     Arrange things so that the next call to ident_walk(1) will return the
 *     user after the given user.
 *
 *     Optional.  If not provided, we do ident_walk(0) followed by ident_walk(1)
 *     until we arrive at the given login.
 */

#define HAVEIDENT_SEEK		/* Default is fine */

/* FUNCTION: ident_newfname(pwd, fname)
 *
 *     Change the user's full name.  pwd will have the full set of old
 *     ident information including login name.
 *
 *     Optional.  If not provided, fullnames cannot be editted through
 *     Backtalk.
 */

#define HAVEIDENT_NEWFNAME

/* FUNCTION: ident_newgid(pwd, gid)
 *
 *     Change the user's primary group id.  pwd will have the full set of old
 *     ident information including login name.
 *
 *     Optional.  If not provided, gids cannot be editted through Backtalk.
 */

#define HAVEIDENT_NEWGID

/* FUNCTION: ident_add(login,uid,gid,fname,dir)
 *
 *     Add a new entry into the identity database.
 *
 *     Optional.  If not provided, users cannot be created.  Not required if
 *     ident database is shared with the auth database.
 */

#define HAVEIDENT_ADD
 
/* FUNCTION: ident_del(login)
 *
 *     Delete the named user from ident database.
 *
 *     Optional.  If not provided, users cannot be deleted.  Not required if
 *     WANT_DELIDENT is not defined, which occurs when the ident database is
 *     shared with the auth database.
 */

#define HAVEIDENT_DEL
