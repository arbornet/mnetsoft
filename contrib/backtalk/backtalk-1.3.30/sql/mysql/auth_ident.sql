# SQL Command Definitions:  MySQL / auth_sql and ident_sql

# This version does not actually save the user directory path.  It will be
# generated on the fly from the user login if we need it.

# MACRO DEFINITIONS 
#   We set the table name and field names here so they are easy to edit and
#   can be referenced by the group table definition.

&tab_auth = bt_user
&col_auth_login = login
&col_auth_pass = password

&tab_ident = bt_user
&col_ident_login = login
&col_ident_pass = password
&col_ident_uid = uid
&col_ident_gid = gid
&col_ident_fname = fullname

# Create the authentication database
auth_create()
{
# IDENT_CREATE does this for us
}

# Create the identity database
ident_create()
{
    -DROP TABLE &tab_ident;
    CREATE TABLE &tab_ident (
	   &col_ident_login	&type_login PRIMARY KEY,
	   &col_ident_pass	&type_pass,
	   &col_ident_uid	&type_uid NOT NULL,
	   &col_ident_gid	&type_gid NOT NULL,
	   &col_ident_fname	&type_fname
    );
}


# Given a login ID, return one row with one column containing the encrypted
#  password for the account.
auth_getpass($login)
{
      SELECT &col_ident_pass FROM &tab_auth WHERE &col_ident_login=$'login;
}

# For each known user, return a row containing the login ID in column one.
# This should be sorted into some stable order (probably alphabetical by
# login).  If &n is given, it is the maximum number of rows we really care
# about.
auth_logins($n)
{
      SELECT &col_ident_login FROM &tab_auth
        ORDER BY &col_ident_login
	$[n LIMIT $n];
}

# For each known user, starting with the one after the given one, return a
# row containing the login ID in column one.  Rows should be sorted into
# some stable order (probably alphabetical by login).  If $n is given, it
# is the maximum number of rows we really care about.
auth_logins_after($login,$n)
{
      SELECT &col_ident_login FROM &tab_auth
        WHERE &col_ident_login > $'login
      	ORDER BY &col_ident_login
	$[n LIMIT $n];
}

# Set the given user's encrypted password to the given value.  Return nothing.
auth_newpass($login,$pass)
{
      UPDATE &tab_auth SET &col_ident_pass=$'pass
        WHERE &col_ident_login=$'login;
}

# Add a user to the authentication database
auth_add($login,$pass,$uid,$gid,$fname,$dir)
{
      INSERT INTO &tab_auth
        (&col_ident_login,&col_ident_pass,&col_ident_uid,&col_ident_gid,
	 &col_ident_fname)
        VALUES ($'login, $'pass, $uid, $gid, $'fname);
}

# Delete a user from the authentication database
auth_del($login)
{
      DELETE FROM &tab_auth WHERE &col_ident_login=$'login;
}


# Given a login id, return one row containing the following columns
#  1: encrypted password
#  2: uid number
#  3: gid number
#  4: full name
#  5: directory name or NULL
ident_getpwnam($login)
{
      SELECT &col_ident_pass,&col_ident_uid,&col_ident_gid,
             &col_ident_fname,NULL
        FROM &tab_auth
        WHERE &col_ident_login=$'login;
}

# Given a uid number, return one row containing the following columns
#  1: encrypted password
#  2: login name
#  3: gid number
#  4: full name
#  5: directory name or NULL
ident_getpwuid($uid)
{
      SELECT &col_ident_pass,&col_ident_login,&col_ident_gid,
             &col_ident_fname,NULL
	FROM &tab_auth
        WHERE &col_ident_uid=$uid;
}

# Return a row for every user, with each row containing the following columns:
#  1: login name
#  2: encrypted password
#  3: uid number
#  4: gid number
#  5: full name
#  6: directory name or NULL
# If n is defined, then we are really interested in no more than n results.
ident_users($n)
{
      SELECT &col_ident_login,&col_ident_pass,&col_ident_uid,
             &col_ident_gid,&col_ident_fname,NULL
        FROM &tab_auth
	ORDER BY &col_ident_login
	$[n LIMIT $n];
}


# For each known user, return a row containing the login ID in column one.
# This should be sorted into some stable order (probably alphabetical by
# login).  If n is given, it is the maximum number of rows we really care
# about.
ident_logins($n)
{
      SELECT &col_ident_login FROM &tab_auth
        ORDER BY &col_ident_login
	$[n LIMIT $n];
}

# For each known user, starting with the one after the given one, return a
# row containing the login ID in column one.  Rows should be sorted into
# some stable order (probably alphabetical by login).  If n is given, it
# is the maximum number of rows we really care about.
ident_logins_after($login,$n)
{
      SELECT &col_ident_login FROM &tab_auth
        WHERE &col_ident_login > $'login
      	ORDER BY &col_ident_login
	$[n LIMIT $n];
}

# Change a user's fullname.  Return no rows.
ident_newfname($login,$fname)
{
      UPDATE &tab_auth SET &col_ident_fname=$'fname
        WHERE &col_ident_login=$'login;
}


# Change a user's group id.  Return no rows.
ident_newgid($login,$gid)
{
      UPDATE &tab_auth SET &col_ident_gid=$gid
        WHERE &col_ident_login=$'login;
}

# Add an entry to the identity database
ident_add($login,$uid,$gid,$fname,$dir)
{
}

# Delete an entry from the identity database
ident_del($login)
{
}
