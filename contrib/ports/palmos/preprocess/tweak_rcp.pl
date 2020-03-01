# #!/usr/bin/perl -n
use strict 'subs';
# Use this script to autoupdate the Subversion Number in robotfindskitten.rcp
BEGIN {
    # ok, this is kind of dumb, but way easier than parsing the header file
    `cd preprocess ; make count_messages`;
    $messages = `preprocess/count_messages`;
    print "// This file was AUTOMATICALLY GENERATED.  ";
    print "Edit the .rcp.in file instead.\n";
}
# Replace each occurence of MESSAGES with <number>
s|MESSAGES|$messages|g;
print $_;
