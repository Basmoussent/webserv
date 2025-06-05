#!/usr/bin/perl
print "Content-Type: text/html\r\n\r\n";
print "<html><body><h1>Current Time</h1>";
print "<p>", scalar localtime(), "</p>";
print "</body></html>"; 