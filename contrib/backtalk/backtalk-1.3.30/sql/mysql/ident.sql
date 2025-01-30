# SQL Command Definitions:  MySQL / ident database

# This version does not actually save the user directory path.  It will be
# generated on the fly from the user login if we need it.

# MACRO DEFINITIONS
#   We set the table name and field names here so they are easy to edit and
#   can be referenced by the group table definition.

&tab_ident = bt_user
&col_ident_login = login
&col_ident_pass = password
&col_ident_uid = uid
&col_ident_gid = gid
&col_ident_fname = fullname

# Create the identity database

ident_create()
{
    -DROP TABLE &tab_ident;
    CREATE TABLE &tab_ident (
           &col_ident_login   &type_login PRIMARY KEY,
           &col_ident_uid     &type_uid NOT NULL,
           &col_ident_gid     &type_gid NOT NULL,
           &col_ident_fname   &type_fname
    );
}


# Given a login id, return one row containing the following columns
#  1: encrypted password
#  2: uid number
#  3: gid number
#  4: full name
#  5: directory name or NULL

ident_getpwnam($login)
{
      SELECT NULL,&col_ident_uid,&col_ident_gid,&col_ident_fname,NULL
	FROM &tab_ident
	WHERE &col_ident_login=$'login;
}


# Given a uid number, return one row containing the following columns
#  1: login name
#  2: encrypted password or NULL
#  3: gid number
#  4: full name
#  5: directory name or NULL

ident_getpwuid($uid)
{
      SELECT &col_ident_login,NULL,&col_ident_gid,&col_ident_fname,NULL
	FROM &tab_ident
	WHERE &col_ident_uid=$uid;
}


# Return a row for every user, with each row containing the following columns:
#  1: login name
#  2: encrypted password or NULL
#  3: uid number
#  4: gid number
#  5: full name
#  6: directory name or NULL
# If $n is defined, then we are really interested in no more than &n results.

ident_users($n)
{
      SELECT &col_ident_login,NULL,&col_ident_uid,
	     &col_ident_gid,&col_ident_fname,NULL
        FROM &tab_ident
	ORDER BY &col_ident_login
	$[n LIMIT $n];
}


# For each known user, return a row containing the login ID in column one.
# This should be sorted into some stable order (probably alphabetical by
# login).  If $n is given, it is the maximum number of rows we really care
# about.

ident_logins($n)
{
      SELECT &col_ident_login FROM &tab_ident
        ORDER BY &col_ident_login
	$[n LIMIT $n];
}


# For each known user, starting with the one after the given one, return a
# row containing the login ID in column one.  Rows should be sorted into
# some stable order (probably alphabetical by login).  If &n is given, it
# is the maximum number of rows we really care about.

ident_logins_after($login,$n)
{
      SELECT &col_ident_login FROM &tab_ident
        WHERE &col_ident_login > $'login
      	ORDER BY &col_ident_login
	$[n LIMIT $n];
}


# Change a user's fullname.  Return no rows.

ident_newfname($login,$fname)
{
      UPDATE &tab_ident SET &col_ident_fname=$'fname
	WHERE &col_ident_login=$'login;
}


# Change a user's group id.  Return no rows.

ident_newgid($login,$gid)
{
      UPDATE &tab_ident SET &col_ident_gid=$gid
	WHERE &col_ident_login=$'login;
}


# Add an entry to the identity database

ident_add($login,$uid,$gid,$fname,$dir)
{
      INSERT INTO &tab_ident
        (&col_ident_login,&col_ident_uid,&col_ident_gid,&col_ident_fname)
        VALUES ($'login, $uid, $gid, $'fname);
}


# Delete an entry from the identity database

ident_del($login)
{
      DELETE FROM &tab_ident WHERE &col_ident_login=$'login;
}
