#!/usr/bin/nawk -f -
BEGIN {
	for (i = 0; i < ARGC; i++) twits[ARGV[i]]++
	delete ARGV
	ARGC = 0
	isok = 1
}
/^!/ && NF == 1 {
	sub(/^!/, "", $1)
	isok = !($1 in twits)
	next
}
isok { print }
