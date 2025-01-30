# SQL Command Definitions:  PostgreSQL / group database

# MACRO DEFINITIONS 
#   We set the table name, field names and field sizes here so they are easier
#   to edit.
&tab_grp = bt_group
&col_grp_name = name
&col_grp_gid = gid

&tab_mem = bt_member
&col_mem_gid = gid
&col_mem_login = login

# Create the group database
group_create()
{
    -DROP INDEX idx_log_&tab_mem;
    -DROP INDEX idx_gid_&tab_mem;
    -DROP TABLE &tab_mem;
    -DROP TABLE &tab_grp;
    CREATE TABLE &tab_grp (
	   &col_grp_gid		&type_gid PRIMARY KEY,
	   &col_grp_name	&type_gname UNIQUE NOT NULL
    );
    INSERT INTO &tab_grp (&col_grp_gid, &col_grp_name) VALUES (0,'cfadm');
    INSERT INTO &tab_grp (&col_grp_gid, &col_grp_name) VALUES (1,'user');
    INSERT INTO &tab_grp (&col_grp_gid, &col_grp_name) VALUES (2,'gradm');
    CREATE TABLE &tab_mem (
	   &col_mem_login	&type_login NOT NULL
	   			    REFERENCES &tab_ident(&col_ident_login)
				    ON DELETE CASCADE ON UPDATE CASCADE,
	   &col_mem_gid		&type_gid NOT NULL
	   			    REFERENCES &tab_grp(&col_grp_gid)
				    ON DELETE CASCADE ON UPDATE CASCADE
    );
    CREATE INDEX idx_log_&tab_mem ON &tab_mem(&col_mem_login);
    CREATE INDEX idx_gid_&tab_mem ON &tab_mem(&col_mem_gid);
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
      SELECT 1 FROM &tab_grp,&tab_ident
        WHERE &tab_ident.&col_ident_login=$'login
	  AND &tab_grp.&col_grp_gid=&tab_ident.&col_ident_gid
	  AND &tab_grp.&col_grp_name=$'gname
	UNION SELECT 1 FROM &tab_grp,&tab_mem
          WHERE &tab_grp.&col_grp_name=$'gname
	    AND &tab_mem.&col_mem_login=$'login
	    AND &tab_grp.&col_grp_gid=&tab_mem.&col_mem_gid;
}


# Given a group id and a login name, return a row containing 1, if that
# user is secondarily in that group.  (This is NOT required to detect
# primary group membership).  If the login is not a secondary member of
# the group, no rows are returned.

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
        FROM &tab_grp,&tab_ident
	WHERE &tab_ident.&col_ident_login=$'login
	  AND &tab_grp.&col_grp_gid=&tab_ident.&col_ident_gid
	UNION SELECT &tab_grp.&col_grp_name,&tab_grp.&col_grp_gid
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
      INSERT INTO &tab_mem (&col_mem_login, &col_mem_gid)
	VALUES ($'login, $gid);
}


# Delete a group

group_del($gname)
{
      DELETE FROM &tab_mem
      	WHERE &col_mem_gid IN (SELECT &col_grp_gid FROM &tab_grp
				WHERE &col_grp_name = $'gname);
      DELETE FROM &tab_grp WHERE &col_grp_name = $'gname;
}


# Delete a member from group

group_memdel($gname,$login)
{
      DELETE FROM &tab_mem
      	WHERE &col_mem_login = $'login
	  AND &col_mem_gid IN (SELECT &col_grp_gid FROM &tab_grp
				WHERE &col_grp_name = $'gname);
}
