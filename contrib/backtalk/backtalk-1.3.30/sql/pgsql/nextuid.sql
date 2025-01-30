# SQL Command Definitions:  PostgreSQL / nextuid database

# MACRO DEFINITIONS 
#   We set the name here so it will be easy to edit.
&seq_uid = bt_uid

# Create the nextuid sequence
uid_create()
{
    -DROP SEQUENCE &seq_uid;
    CREATE SEQUENCE &seq_uid MINVALUE 0;
}


# Return the next sequence number, incrementing it.
uid_next()
{
      SELECT NEXTVAL('&seq_uid');
}


# Return the next sequence number, without incrementing it.
uid_curr()
{
      SELECT CURRVAL('&seq_uid');
}
