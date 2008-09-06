#!/usr/bin/perl

use File::Basename;

my ($myself, $path) = fileparse($0);
chdir($path);

require 'helpers.pm';

chdir('..');
chdir('..');

our $SSH = 'ssh -i ../../smpvkey';

open(PDBLOG, 'OUTPUT/pdblog.txt') or die "Could not open pdblog.txt: $!\n";

#Sync us up with the main symbol store
rsync('sourcemod@alliedmods.net:~/public_html/symbols/', '..\\..\\symstore');

#Get version info
my ($version);
$version = Build::ProductVersion(Build::PathFormat('product.version'));
$version .= '.' . Build::Revision('.');

my ($line);
while (<PDBLOG>)
{
	$line = $_;
	$line =~ s/\.pdb/\*/;
	chomp $line;
	Build::Command("symstore add /r /f \"$line\" /s ..\\..\\symstore /t \"SourceMod\" /v \"$version\" /c \"buildbot\"");
}

close(PDBLOG);

#Now that we're done, rsync back.
rsync('../../symstore/', 'sourcemod@alliedmods.net:~/public_html/symbols');

sub rsync
{
	my ($from, $to) = (@_);
	
	Build::Command('rsync -av --delete -e="' . $SSH . '" ' . $from . ' ' . $to);
}
