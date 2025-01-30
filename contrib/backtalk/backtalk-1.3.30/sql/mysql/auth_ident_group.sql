# SQL Command Definitions:  MySQL / merged auth, ident, and group database

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
&col_ident_fname = fullname

&tab_grp = bt_group
&col_grp_name = name
&col_grp_gid = gid

&tab_mem = bt_member
&col_mem_gid = gid
&col_mem_login = login
&col_mem_prime = prime


# Create the authentication database

auth_create()
{
# ident_create does this for us
}


# Create the identity and group databases

ident_create()
{
    -DROP INDEX idx_log_&tab_mem;
    -DROP INDEX idx_gid_&tab_mem;
    -DROP TABLE &tab_mem;
    -DROP TABLE &tab_grp;
    -DROP TABLE &tab_ident;
    CREATE TABLE &tab_ident (
	   &col_ident_login	&type_login PRIMARY KEY,
	   &col_ident_pass	&type_pass,
	   &col_ident_uid	&type_uid NOT NULL,
	   &col_ident_fname	&type_fname
    );
    CREATE TABLE &tab_grp (
           &col_grp_gid         &type_gid PRIMARY KEY,
           &col_grp_name        &type_gname UNIQUE NOT NULL
    );
    INSERT INTO &tab_grp (&col_grp_gid, &col_grp_name) VALUES (0,'cfadm');
    INSERT INTO &tab_grp (&col_grp_gid, &col_grp_name) VALUES (1,'user');
    INSERT INTO &tab_grp (&col_grp_gid, &col_grp_name) VALUES (2,'gradm');
    CREATE TABLE &tab_mem (
           &col_mem_login       &type_login NOT NULL,
           &col_mem_gid         &type_gid NOT NULL,
           &col_mem_prime       &type_flag NOT NULL,
	   INDEX(&col_mem_login),
	   INDEX(&col_mem_gid)
    );
}

group_create()
{
# ident_create does this for us
}


# Given a login ID, return one row with one column containing the encrypted
#  password for the account.

auth_getpass($login)
{
      SELECT &col_ident_pass FROM &tab_ident WHERE &col_ident_login = $'login;
}


# For each known user, return a row containing the login ID in column one.
# This should be sorted into some stable order (probably alphabetical by
# login).  If &n is given, it is the maximum number of rows we really care
# about.

auth_logins($n)
{
      SELECT &col_ident_login FROM &tab_ident
        ORDER BY &col_ident_login
	$[n LIMIT $n];
}


# For each known user, starting with the one after the given one, return a
# row containing the login ID in column one.  Rows should be sorted into
# some stable order (probably alphabetical by login).  If $n is given, it
# is the maximum number of rows we really care about.

auth_logins_after($login,$n)
{
      SELECT &col_ident_login FROM &tab_ident
        WHERE &col_ident_login > $'login
      	ORDER BY &col_ident_login
	$[n LIMIT $n];
}


# Set the given user's encrypted password to the given value.  Return nothing.

auth_newpass($login,$pass)
{
      UPDATE &tab_ident SET &col_ident_pass = $'pass
        WHERE &col_ident_login = $'login;
}


# Add a user to the authentication database

auth_add($login,$pass,$uid,$gid,$fname,$dir)
{
      INSERT INTO &tab_ident
        (&col_ident_login, &col_ident_pass, &col_ident_uid, &col_ident_fname)
        VALUES ($'login, $'pass, $uid, $'fname);
      INSERT INTO &tab_mem
        (&col_mem_login, &col_mem_gid, &col_mem_prime)
	VALUES ($'login, $gid, 1);
}


# Delete a user from the authentication database

auth_del($login)
{
      DELETE FROM &tab_ident WHERE &col_ident_login = $'login;
      DELETE FROM &tab_mem WHERE &col_mem_login = $'login;
}


# Given a login id, return one row containing the following columns
#  1: encrypted password
#  2: uid number
#  3: gid number
#  4: full name
#  5: directory name or NULL

ident_getpwnam($login)
{
      SELECT &tab_ident.&col_ident_pass, &tab_ident.&col_ident_uid,
      	     &tab_mem.&col_mem_gid, &tab_ident.&col_ident_fname, NULL
        FROM &tab_ident, &tab_mem
        WHERE &tab_ident.&col_ident_login = $'login
	  AND &tab_mem.&col_mem_login = $'login
	  AND &tab_mem.&col_mem_prime = 1;
}


# Given a uid number, return one row containing the following columns
#  1: encrypted password
#  2: login name
#  3: gid number
#  4: full name
#  5: directory name or NULL

ident_getpwuid($uid)
{
      SELECT &tab_ident.&col_ident_pass, &tab_ident.&col_ident_login,
      	     &tab_mem.&col_mem_gid, &tab_ident.&col_ident_fname, NULL
	FROM &tab_ident, &tab_mem
        WHERE &tab_ident.&col_ident_uid = $uid;
	  AND &tab_mem.&col_mem_login = &tab_ident.&col_ident_login
	  AND &tab_mem.&col_mem_prime = 1;
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
      SELECT &tab_ident.&col_ident_login, &tab_ident.&col_ident_pass,
      	     &tab_ident.&col_ident_uid, &tab_mem.&col_mem_gid,
	     &tab_ident.&col_ident_fname, NULL
        FROM &tab_ident, &tab_mem
	WHERE &tab_mem.&col_mem_login = &tab_ident.&col_ident_login
	  AND &tab_mem.&col_mem_prime = 1
	ORDER BY &col_ident_login
	$[n LIMIT $n];
}


# For each known user, return a row containing the login ID in column one.
# This should be sorted into some stable order (probably alphabetical by
# login).  If n is given, it is the maximum number of rows we really care
# about.

ident_logins($n)
{
      SELECT &col_ident_login FROM &tab_ident
        ORDER BY &col_ident_login
	$[n LIMIT $n];
}


# For each known user, starting with the one after the given one, return a
# row containing the login ID in column one.  Rows should be sorted into
# some stable order (probably alphabetical by login).  If n is given, it
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
      UPDATE &tab_ident SET &col_ident_fname = $'fname
        WHERE &col_ident_login = $'login;
}


# Change a user's group id.  Return no rows.

ident_newgid($login,$gid)
{
      UPDATE &tab_mem SET &col_mem_gid = $gid
        WHERE &col_mem_login = $'login
	  AND &col_mem_prime = 1;
}


# Add an entry to the identity database

ident_add($login,$uid,$gid,$fname,$dir)
{
}


# Delete an entry from the identity database

ident_del($login)
{
}


# Given a group id, return a row with the matching group name in column one.

group_gid_to_name($gid)
{
      SELECT &col_grp_name FROM &tab_grp WHERE &col_grp_gid=$gid;
}


# Given a group name, return a row with the matching group id in column one.

group_name_to_gid($gname)
{
      SELECT &col_grp_gid FROM &tab_grp WHERE &col_grp_name=$'gname;
}


# Given a group name and a login name, return a row containing 1
# id if that user is primarily or secondarily in that group.  If not,
# return no rows.

group_in_name($login,$gname)
{
      SELECT 1 FROM &tab_grp,&tab_mem
	WHERE &tab_grp.&col_grp_name=$'gname
	  AND &tab_mem.&col_mem_login=$'login
	  AND &tab_grp.&col_grp_gid=&tab_mem.&col_mem_gid;
}


# Given a group id and a login name, return a row containing 1, if that
# user is secondarily in that group.  (This is NOT required to detect
# primary group membership though it does in this case).  If the login is
# not a secondary member of the group, no rows are returned.

group_in_gid($login,$gid)
{
      SELECT 1 FROM &tab_mem
	WHERE &tab_mem.&col_mem_login=$'login
	  AND &tab_mem.&col_mem_gid=$gid;
}


# For each defined group, return the following columns
#   1  group name
#   2  group id
# If $n is greater than zero, it is the maximum number of groups we really
# care about.

group_all($n)
{
      SELECT &col_grp_name,&col_grp_gid FROM &tab_grp
	ORDER BY &col_grp_gid
	$[n LIMIT $n];
}


# For each group of which the given user is a member, return the following
# columns
#   1  group name
#   2  group id
# If $n is greater than zero, it is the maximum number of groups we really
# care about.

group_mine($login,$n)
{
      SELECT &tab_grp.&col_grp_name,&tab_grp.&col_grp_gid
	FROM &tab_grp,&tab_mem
	WHERE &tab_mem.&col_mem_login=$'login
	  AND &tab_grp.&col_grp_gid=&tab_mem.&col_mem_gid
        ORDER BY &tab_grp.&col_grp_gid
        $[n LIMIT $n];
}


# Create a new group

group_add($gid,$gname)
{
      INSERT INTO &tab_grp (&col_grp_name, &col_grp_gid)
        VALUES ($'gname, $gid);
}


# Add a secondary member to a group

group_memadd($gid,$login)
{
      INSERT INTO &tab_mem (&col_mem_login, &col_mem_gid, &col_mem_prime)
	VALUES ($'login, $gid, 0);
}


# Delete a group

group_del($gname)
{
      SELECT @gid:=&col_grp_gid FROM &tab_grp WHERE &col_grp_name = $'gname;
      DELETE FROM &tab_mem WHERE &col_mem_gid = @gid;
      DELETE FROM &tab_grp WHERE &col_grp_name = $'gname;
}


# Delete a member from group

group_memdel($gname,$login)
{
      SELECT @gid:=&col_grp_gid FROM &tab_grp WHERE &col_grp_name = $'gname;

      DELETE FROM &tab_mem
      	WHERE &col_mem_login = $'login
	  AND &col_mem_gid = @gid;
}
