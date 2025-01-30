# SQL Command Definitions:  MySQL / nextuid database

# We simulate a SEQUENCE using the LAST_INSERT_ID() function.

# MACRO DEFINITIONS 
#   We set the name here so it will be easy to edit.
&tab_seq_uid = bt_uid

# Create the nextuid sequence
uid_create()
{
    -DROP TABLE &tab_seq_uid;
    CREATE TABLE &tab_seq_uid (id INT NOT NULL);
    INSERT INTO &tab_seq_uid VALUES (0);
}


# Return the next sequence number, incrementing it.
uid_next()
{
      UPDATE &tab_seq_uid SET id=last_insert_id(id+1);
      SELECT last_insert_id() FROM &tab_seq_uid;
}


# Return the next sequence number, without incrementing it.
uid_curr()
{
      SELECT id FROM &tab_seq_uid;
}
