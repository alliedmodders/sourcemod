#!/usr/bin/perl

use strict;
use Cwd;
use File::Basename;

my ($myself, $path) = fileparse($0);
chdir($path);

require 'helpers.pm';

chdir(Build::PathFormat('../builder'));
if ($^O eq "linux")
{
    Build::Command('make clean');
    Build::Command('make');
}
else
{
    Build::Command($ENV{'MSVC7'} . ' /Rebuild Release builder.csproj');
    Build::Command('move ' . Build::PathFormat('bin/Release/builder.exe') . ' .');
}

die "Unable to build builder tool!\n" unless -e 'builder.exe';

