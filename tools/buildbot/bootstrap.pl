#!/usr/bin/perl

my $incoming = shift;

die "Invalid source folder.\n" unless $incoming ne '';

require $incoming . '/tools/buildbot/helpers.pm';

chdir(Build::PathFormat($incoming . '/tools/builder'));
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

