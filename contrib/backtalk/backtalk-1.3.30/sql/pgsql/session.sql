# SQL Command Definitions:  PostgreSQL / session database

# This defines a session table, which keeps track of currently active sessions,
# and a one-row salt table with gathers entropy for future session ID
# generation.

# MACRO DEFINITIONS
#   We set the table name and field names here so they are easy to edit.

&tab_sess = bt_sess
&col_sess_sid = sid
&col_sess_login = login
&col_sess_time = time
&col_sess_ip = ip

# We no longer save salt in the database
&tab_salt = bt_salt
&seq_salt = bt_sesseq

&type_sid = char(24)

# Create the session database

sess_create()
{
    -DROP TABLE &tab_sess;
    CREATE TABLE &tab_sess (
	&col_sess_sid	&type_sid PRIMARY KEY,
	&col_sess_login	&type_login,
	&col_sess_time	&type_time,
	&col_sess_ip	&type_ip
    );
    -DROP TABLE &tab_salt;
    -DROP SEQUENCE &seq_salt;
}


# Given a session ID, return one row with the following columns:
#   1:  login id
#   2:  last access time
#   3:  last ip address
# If it doesn't exist, return no rows.

sess_get($sid)
{
      SELECT &col_sess_login, &col_sess_time, &col_sess_ip
        FROM &tab_sess WHERE &col_sess_sid=$'sid;
}


# Set the last access time of a session to a new value.  Also stir the salt.

sess_refresh($sid,$time)
{
	UPDATE &tab_sess
	  SET &col_sess_time = $time
	  WHERE &col_sess_sid = $'sid;
}


# Create a new session.

sess_new($sid,$login,$time,$ip)
{
	INSERT INTO &tab_sess
	    (&col_sess_sid, &col_sess_login, &col_sess_time, &col_sess_ip)
          VALUES ($'sid, $'login, $time, $'ip);
}


# Kill a particular session.

sess_kill($sid)
{
	DELETE FROM &tab_sess WHERE &col_sess_sid = $'sid;
}


# Reap all sessions predating the given time.

sess_reap($time)
{
	DELETE FROM &tab_sess WHERE &col_sess_time < $time;
}
