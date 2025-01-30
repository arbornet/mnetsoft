# Copyright 2005, Jan D. Wolter, All Rights Reserved.

# ($type,$datestr)= parse_date($strref)
#   Given a reference to a string, return an initial substring that corresponds
#   to a date, stripping it out of the string.  For Picospan compatibiity
#   we do not require date strings to be quoted, but because the range of
#   date syntax we accept is broader than Picospan, parsing out the date
#   becomes a bit iffy.  This should work for anything reasonably normal.
#   Type is
#      'D'    quoted string or multi-word date string
#      'W'    single word.

sub parse_date {
    my ($strref)= @_;
    my ($str,$term,$word,$delim,$type);

    # Delete leading spaces
    $$strref =~ s/^\s*//;

    # We like quoted date strings
    if ($$strref =~ s/^"((?:[^"\\]|\\.)*)("|$)\s*// or
        $$strref =~ s/^'((?:[^'\\]|\\.)*)('|$)\s*//)
    {
	# We don't recognize many escape characters in date strings
	$str= $1;
	$str=~ s/\\t/ /g;
	$str=~ s/\\n/ /g;
	$str=~ s/\\([^\\])/\1/g;
	$str=~ s/\\\\/\\/g;
	return ('D', $str);
    }

    # Otherwise first space-delimited word is certainly part of our date string
    $type= 'W';
    if ($$strref =~ s/^(\S+)//)
    {
	$str= $1;
	$$strref =~ s/^\s*//;
	# if the was whole string, return the word we found
	return ($type, $str) if $$strref !~ /\S/;
    }
    else
    {
	# if string was blank, return undef
	return (undef,undef);
    }

    # Keep clipping out date-like elements until we find something undate-like
    # or until we find the end of the string
    while ($$strref =~ s {
	    ^(
	       # numbers:  "17" or "-1" or "+23"
	       (?:\+|-)?\s*\d+
	     |
	       # times: "3:23" or "12:33:01" or "1:22 pm"
	       [012]?\d:[0-5]\d(?::[0-5]\d)?
	       (?:\s*(?:am|pm|noon|midnight|midnite))?
	     |
	       # times: "1 am" or "12 noon"
	       [012]?\d\s*(?:am|pm|noon|midnight|midnite)
	     |
	       # months:  "Jan" or "January"
	       jan(?:uary)? | feb(?:ruary)? | mar(?:ch)? | apr(?:il)? |
	       may | june? | july? | aug(?:ust)? | sept?(?:ember)? |
	       oct(?:ober)? | nov(?:ember)? | dec(?:ember)?
	     |
	       # weekdays:  "Sun" or "Sunday"
	       sun(?:day)? | mon(?:day)? | tue(?:sday)? | wed(?:nesday)? |
	       thur?(?:sday)? | fri(?:day)? | sat(?:urday)?
	     |
	       # intervals:  "weeks" or "month"
	       years? | months? | weeks? | days? | hours? |
	       min(?:ute)s? | sec(?:ond)s?
	     |
	       # other words:
	       yesterday | tomorrow | today |
	       beginning | start | beginning | end |
	       ago | hence | last | next | time | the | of
	   )
	   # delimiter - we don't accept '-' as a delimiter, because we'd
	   # end up grabbing item ranges like 2-12.  Dashes in first word
	   # or quoted strings should still work though.
	   ([\s,;/.()]+|$)

	}{}ix )
    {
	$term= $1;
	$delim= $2;
	if ($delim =~ /^(\S*)(?:\s+)(.*)$/ or $delim =~ /^$/)
	{
	    # got whitespace delimiter:  end word and add it to string
	    $str.= ' ' if defined($str);
	    $str.= $word.$term.$1;
	    $word= $2;
	    $type= 'D';
	}
	else
	{
	    # got non-white delimiter:  add term to word
	    $word.= $term.$delim;
	}
    }

    # Put any undigested word back on string
    $$strref= $word.$$strref if defined($word);

    return ($type, $str);
}

1;
