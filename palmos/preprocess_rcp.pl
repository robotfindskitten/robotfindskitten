# #!/usr/bin/perl -n
use strict 'subs';
# Use this script to autoupdate the Subversion Number in robotfindskitten.rcp
BEGIN {
    # Find the "#define MESSAGES <number>..." line and extract <number>
    $FILE = "<../src/messages.h";
    open FILE or die "can't open messages.h: $!";
    @lines = <FILE>;		# honking big array
    @glines = grep /^\s*\#define\s+MESSAGES\s/, @lines;
    $def = @glines[$#glines];
    $def =~ m|^\s*\#define\s+MESSAGES\s+([0-9]*)|;
    $messages = $1;
    print "// This file was AUTOMATICALLY GENERATED.  ";
    print "Edit the .rcp.in file instead.\n";
}
# Replace each occurence of MESSAGES with <number>
s|MESSAGES|$messages|g;
print $_;
