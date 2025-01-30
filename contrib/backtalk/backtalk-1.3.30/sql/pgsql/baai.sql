# SQL Command Definitions:  PostgreSQL / backtalk archive index database

# MACRO DEFINITIONS
#   We set the table name and field names here so they are easy to edit and
#   can be referenced by the group table definition.

&tab_baai = bt_attach
&col_baai_handle = handle
&col_baai_type = mimetype
&col_baai_size = size
&col_baai_date = created
&col_baai_desc = descrip
&col_baai_access = acc
&col_baai_conf = conf
&col_baai_login = login
&col_baai_uid = uid
&col_baai_link = link

# Create the backtalk attachment archive index

baai_create()
{
    -DROP TABLE &tab_baai;
    CREATE TABLE &tab_baai (
           &col_baai_handle   &type_athand PRIMARY KEY,
           &col_baai_type     &type_mimetype,
	   &col_baai_size     int4,
	   &col_baai_date     &type_time,
	   &col_baai_desc     varchar(800),
	   &col_baai_access   int2,
	   &col_baai_conf     &type_confname,
           &col_baai_login    &type_login,
           &col_baai_uid      &type_uid,
           &col_baai_link     int2
    );
}


# Given a handle, return the fields in the following order
#  1: type
#  2: size
#  3: login
#  4: uid
#  5: conf
#  6: access
#  7: date
#  8: desc
#  9: link

baai_get($handle)
{
      SELECT &col_baai_type,&col_baai_size,&col_baai_login,&col_baai_uid,
	  &col_baai_conf,&col_baai_access,&col_baai_date,&col_baai_desc,
	  &col_baai_link
	FROM &tab_baai
	WHERE &col_baai_handle=$'handle;
}

# Return all index file entires '
#  1: type
#  2: size
#  3: login
#  4: uid
#  5: conf
#  6: access
#  7: date
#  8: desc
#  9: link
# 10: handle

baai_list()
{
      SELECT &col_baai_type,&col_baai_size,&col_baai_login,&col_baai_uid,
          &col_baai_conf,&col_baai_access,&col_baai_date,&col_baai_desc,
          &col_baai_link,&col_baai_handle
        FROM &tab_baai;
}

# Add a database entry

baai_add($handle,$type,$size,$login,$uid,$conf,$access,$date,$desc,$link)
{
      INSERT INTO &tab_baai
        (&col_baai_handle,&col_baai_type,&col_baai_size,&col_baai_date,
	 &col_baai_desc,&col_baai_access,&col_baai_conf,&col_baai_login,
	 &col_baai_uid, &col_baai_link)
        VALUES ($'handle, $'type, $size, $date, $'desc, $access, $'conf,
	        $'login, $uid, $link);
}

# Edit a database entry '

baai_edit($handle,$type,$size,$login,$uid,$conf,$access,$date,$desc,$link)
{
      UPDATE &tab_baai
	SET &col_baai_type=$'type,
	    &col_baai_size=$size,
	    &col_baai_date=$date,
	    &col_baai_desc=$'desc,
	    &col_baai_access=$access,
	    &col_baai_conf=$'conf,
	    &col_baai_login=$'login,
	    &col_baai_uid=$uid,
	    &col_baai_link=$link
        WHERE &col_baai_handle=$'handle;
}

# Set the link field '

baai_link($handle)
{
      UPDATE &tab_baai
	SET &col_baai_link=&col_baai_link+1
        WHERE &col_baai_handle=$'handle;
}

# Delete a database entry '

baai_del($handle)
{
      DELETE FROM &tab_baai WHERE &col_baai_handle=$'handle;
}
