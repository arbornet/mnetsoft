# SQL Command Definitions:  MySQL / auth database

# MACRO DEFINITIONS
#   We set the table name and field names here so they are easy to edit.

&tab_auth = bt_passwd
&col_auth_login = login
&col_auth_pass = password

# Create the authentication database

auth_create()
{
    -DROP TABLE &tab_auth;
    CREATE TABLE &tab_auth (
	   &col_auth_login	&type_login PRIMARY KEY,
	   &col_auth_pass	&type_pass
    );
}


# Given a login ID, return one row with one column containing the encrypted
#  password for the account.

auth_getpass($login)
{
      SELECT &col_auth_pass FROM &tab_auth WHERE &col_auth_login=$'login;
}

# For each known user, return a row containing the login ID in column one.
# This should be sorted into some stable order (probably alphabetical by
# login).  If &n is given, it is the maximum number of rows we really care
# about.

auth_logins($n)
{
      SELECT &col_auth_login FROM &tab_auth
        ORDER BY &col_auth_login
	$[n LIMIT $n];
}

# For each known user, starting with the one after the given one, return a
# row containing the login ID in column one.  Rows should be sorted into
# some stable order (probably alphabetical by login).  If $n is given, it
# is the maximum number of rows we really care about.

auth_logins_after($login,$n)
{
      SELECT &col_auth_login FROM &tab_auth
        WHERE &col_auth_login > $'login
      	ORDER BY &col_auth_login
	$[n LIMIT $n];
}

# Set the given user's encrypted password to the given value.  Return nothing.

auth_newpass($login,$pass)
{
      UPDATE &tab_auth SET &col_auth_pass=$'pass WHERE &col_auth_login=$'login;
}

# Add a user to the authentication database

auth_add($login,$pass,$uid,$gid,$fname,$dir)
{
      INSERT INTO &tab_auth (&col_auth_login,&col_auth_pass)
        VALUES ($'login, $'pass);
}

# Delete a user from the authentication database

auth_del($login)
{
      DELETE FROM &tab_auth WHERE &col_auth_login=$'login;
}
