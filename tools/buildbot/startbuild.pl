#!/usr/bin/perl

use File::Basename;

my ($myself, $path) = fileparse($0);
chdir($path);
chdir('..');
chdir('..');

my ($cmd, $output);

$cmd = 'tools/builder/builder.exe build.cfg 2>&1';

if ($^O eq "linux")
{
    $cmd = 'mono ' . $cmd;
}

system($cmd);

if ($? == -1)
{
    die "Build failed: $!\n";
}
elsif ($^O eq "linux" and $? & 127)
{
    die "Build died :(\n";
}
elsif ($? >> 8 != 0)
{
    die "Build failed with exit code: " . ($? >> 8) . "\n";
}
else
{
    exit(0);
}
