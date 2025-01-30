.TH GATE 1  "6 June 1995"
.SH NAME
gate \- gather text input for PicoSpan or Yapp with word-wrapping
.SH SYNOPSIS
.B gate
[--\fI<option>\fR...]
.I file
.SH DESCRIPTION
.I Gate
is a word-wrapping input gatherer designed to be used with the
PicoSpan or Yapp conferencing programs. (by Marcus Watts and Dave Thaler
respectively).
It behaves very much like the built-in text gatherers, with the exception
that it automatically word-wraps as you type,
provides integrated spell checking,
and has a few other minor improvements.
.LP
Though it is possible to run 
.I gate
directly (and it might have some application in shell scripts that want to
do nicer text gathering), normally you would allow the conferencing system
to run it for you, by putting the following PicoSpan (or Yapp) commands in
your
.B .cfonce
file:
.LP
.RS
.nf
set edalways
define editor "gate"
.fi
.RE
.LP
The latter command should give the full path of the
.I gate
command, if it is not in your path.
.LP
.I Gate
runs in cbreak mode, but it carefully
simulates all the usual unix line-editting keys, so whatever
backspace, word-erase, line-kill, end-of-file, and reprint keys
you have defined with
.IR stty (1)
will work as usual (even with tabs in your text).
.LP
Gate allows you to backspace back onto previous lines (so long as those
lines are not longer than your screen width).
For more elaborate editing us the
.I :edit
or 
.I :/
commands.
.LP
In addition to the standard keys, typing control-L will redisplay the entire
body of text entered so far.
.LP
Text entry is normally terminated by either a dot (.) typed in the first
column, or the usual unix end-of-file character.
.SH "COLON COMMANDS"
The following special commands can be entered at the beginning of any line.
All of them can be abbreviated.
.IP ":clear" 1i
Empty out the text buffer, discarding everything entered so far, and
restart text entry with a clean slate.
.IP ":edit" 1i
Start up the editor on the text.
The environment variable
.B EDITOR
selects which editor to use.
When you exit the editor, text entry will be continued.
A colon alone on a line will also start the editor.
.IP ":empty" 1i
This is the same as the
.I :clear
command.
.IP ":exit [!]" 1i
This is the same as the
.I :quit
command.
.IP ":help" 1i
Print a short help message.
.IP ":quit [!]" 1i
Terminate text entry, and discard the response without entering it.
Normally, it will ask for confirmation that you really want to do this.
If you give a "!" as an argument, it will skip the confirmation request.
#ifndef RIVER
.IP ":ok" 1i
Terminate text entry, and ask if you want to enter the response or not.
#endif
.IP ":read [-s] <file>" 1i
Append the named file to the text you have entered so far.
Normally unprintable characters will be stripped out of the file as it
is read.  If the
.I -s
flag is given, they will be left in.
.IP ":set [<option>...]" 1i
Without arguments, this command prints the current values of the various
settable options for
.IR gate .
If arguments are given, those options are set.  See below for a list of
options.
#ifndef NO_SPELL
#ifdef IISPELL
.IP ":spell [!]" 1i
#else
.IP ":spell" 1i
#endif
Run a spell check on the current text.
You will be shown each misspelled word in context,
#if defined(IISPELL) || defined(GISPELL)
along with a numbered list of guesses of a correct spelling of the word,
and you will be asked for a replacement.
You can either type in a replacement of your own,
or enter the number of a guess you want to use.
Typing
.B #
(pound)
reprints the list of possible replacements.
#else
and be asked for a replacement.
#endif
If instead of entering a replacement you simply hit return,
all instances of the word will be left unchanged.
If you type a
.B +
(plus)
then all instances of the word will be left unchanged, and it will be added
into your private dictionary so it will be recognized as being
correctly spelled in future spell checks.
If you type a
.B \
(backslash),
then the spell check will be cancelled.
A
.B ?
(question mark)
prints help.
#ifdef IISPELL
Normally, if you run
.I ":spell"
a second time, words you ignored in the first pass will still be ignored
in the second pass.  Putting a
.B !
(exclamation point) on the end of the
.I ":spell"
command causes it to restart with a clean slate.
#endif
#endif
.IP ":visual" 1i
This is the same as the
.I :edit
command.
.IP ":version" 1i
Print out the current
.I gate
version number.
.IP ":write <file>" 1i
Save a copy of the current text buffer in the named file.
.I :edit
command.
.IP ":!<cmd>" 1i
Do a shell escape to execute a unix command.  The colon may be omitted.
.IP ":|<cmd>" 1i
Filter the current text through the given unix command.  The command will
be fed the current text on standard input, and whatever appears on standard
output will replace the contents of the text file.  This is normally used
to pipe through formatting programs.
.IP ":/<pattern>/<replacement>/" 1i
Each occurance of the given pattern in the text entered so far
will be replaced by the given replacement.
As each occurance is found, you asked to confirm the substitution.
Typing "y" does the substitution,
typing "n" skips the substitution,
typing "a" does the substition and all others without further prompting,
and typing "q" stops the scan immediately with no further substitutions.
Both the pattern and the replacement may include the characters "\\n"
which represents a newline character.  This makes it possible to join
and break lines.  A "\\\\" indicates a backslash character, and
a "\\/" indicates a slash.
The terminating slash on the command may be omitted.
Note that this is intended only for simple editting.  For complex editting
tasks, use the
.I ":edit"
command to start up an editor.
.IP ":substitute /<pattern>/<replacement>/" 1i
Equivalent to the ":/" command.
.SH "OPTIONS"
Options may be set either
on the command line (with a ``--'' prefix),
by the
.I :set
command described above, or by putting them in the
.B GATEOPTS
environment variable.
For example, from the
.IR csh (1)
shell you could do:
.RS
.nf
setenv GATEOPTS "nonovice maxcol=70"
.fi
.RE
or from
.IR bbs (1)
you could do:
.RS
.nf
define GATEOPTS 256 "nonovice maxcol=70"
.fi
.RE
.LP
Options currently supported are listed below.
Default settings are installation dependent.
#ifndef RIVER
.IP "[no]askok" 1i
If askok is set,
.I gate
always asks if it is OK to enter this response.
Otherwise it only asks if you do a
.I :ok
command.
.I Askok
is (more or less) implied by the
.I spell
or
.I askspell
options.
#endif
.IP "[no]backwrap" 1i
If the
.I backwrap
is turned on, backspacing in the first column will move you
to the end of the previous line.
If the terminal supports it, and the previous line of the text file is
the previous line of the screen, gate will move the cursor up into that line.
Otherwise, however, it reprints the line.
This behavior is a bit weird and confusing to people who expect a full
visual editor, it, so it may be good to disable this option for beginners.
Note that
.I backwrap
will not work if the previous line is more than maxcol columns long.
.IP "cmdchar=<char>" 1i
.I Cmdchar
specifies the character that is used at the begining of an input line to
indicate that the rest of the line is a command.
The default is a colon (:).
.IP "hotcol=<int>" 1i
.I Hotcol
specifies the last column in which spaces may be entered.  If you type a space
beyond this column, you will be instantly moved to the next line.
The length of your
.I prompt
is included in your line length.
Normally
.I hotcol
is set just slightly smaller than
.IR maxcol .
If
.I hotcol
is larger than
.IR maxcol ,
it has no effect.
#ifdef NO_SPELL
#ifndef GISPELL
.IP "language=<name>" 1i
#ifdef IISPELL
.I Language
can be set to the name of any of the installed foreign language dictionaries
on your system.
The default is LANG_DEFAULT.
#else
If
.I language
is set to "british", the spellchecker will check for British spelling.
Otherwise, American spelling is checked.
#endif
#endif
#endif
.IP "maxcol=<int>" 1i
.I Maxcol
specifies the last column in which any character may be entered.
If you attempt to type a word extending past this column, it will be moved
onto the next line.
The length of your
.I prompt
is included in your line length.
Normally it should be no larger than 79, since typing in the 80th column
confuses some terminals.
It can be set to a value greater than screen width of your terminal with the
.I :set
command, but not with the GATEOPTS.
.IP "[no]novice" 1i
If
.I novice
is set,
.I gate
will print additional help messages if you commit any of several common
novice errors, like typing an input line with just the work "quit" on it.
.IP "outdent=<int>" 1i
When a response is displayed by PicoSpan or Yapp,
each line has a space prepended.
This will indent most lines one column, but lines starting with a tab will
be unchanged.
The
.I outdent
option allows
.I gate
to adjust the positions of its tabstops to correct for this.
Effectively, it does tabbing as if the screen started
.I outdent
columns to the left of the end of the
.IR prompt .
.IP "prompt=<string>" 1i
Normally
.I gate
prints a > prompt for each line.
The prompt can be set to any string, including a null string.
It is slightly preferable to use a prompt whose length is equal to
.IR outdent ,
since this gives a more WYSIWYG display, but this is by no means necessary.
.IP "[no]secure" 1i
If
.I secure
is set, the buffer file being editted will be kept depermitted as much as
possible, to keep people from reading your text before you are finished with
it.  If
.I nosecure
is set, the buffer file will normally be readable to others.
#ifndef NO_SPELL
.IP "[no|ask]spell" 1i
If
.I spell
is set, the spellchecker will automatically be started when you exit.
If 
.I askspell
is set, you will be asked if you want to check spelling when you exit.
#endif
.SH AUTHOR
Jan Wolter
.SH "SEE ALSO"
#ifdef YAPP
.IR yapp (1),
#else
.IR bbs (1),
#endif
.IR vi (1),
.IR pico (1),
.IR stty (1),
#ifndef NO_SPELL
#if defined(IISPELL) || defined(GISPELL)
.IR ispell (1)
#else
.IR spell (1)
#endif
#endif
