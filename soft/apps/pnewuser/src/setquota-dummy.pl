#!/usr/bin/env perl
#
# Copyright (C) 2010  Cyberspace Communications, Inc.
# All Rights Reserved
#
# $Id: setquota-dummy.pl 1006 2010-12-08 02:38:52Z cross $
#
# A simple sink for systems that don't set quotas.
#
use warnings;
use strict;
while (sysread(STDIN, my $buffer, 128) > 0) { }
