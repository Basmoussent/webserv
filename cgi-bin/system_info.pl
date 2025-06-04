#!/usr/bin/perl

use strict;
use warnings;

print "Content-Type: text/html\n\n";
print "<html>\n";
print "<head><title>System Information</title></head>\n";
print "<body>\n";
print "<h1>System Information</h1>\n";
print "<ul>\n";
print "<li><strong>Perl Version:</strong> $^V</li>\n";
print "<li><strong>Operating System:</strong> $^O</li>\n";
print "<li><strong>Process ID:</strong> $$</li>\n";
print "<li><strong>Script Name:</strong> $0</li>\n";
print "<li><strong>Current Directory:</strong> " . `pwd` . "</li>\n";
print "</ul>\n";
print "<p>Environment Variables:</p>\n";
print "<ul>\n";
foreach my $key (sort keys %ENV) {
    print "<li><strong>$key:</strong> $ENV{$key}</li>\n";
}
print "</ul>\n";
print "</body>\n";
print "</html>\n";
