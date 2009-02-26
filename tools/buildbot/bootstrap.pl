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
    Build::Command('"' . $ENV{'VC9BUILDER'} . '" /rebuild builder.csproj Release');
    Build::Command('move ' . Build::PathFormat('bin/Release/builder.exe') . ' .');
}

die "Unable to build builder tool!\n" unless -e 'builder.exe';

#Go back to main source dir.
chdir(Build::PathFormat('../..'));

#Get the source path.
my ($root) = getcwd();

#Create output folder if it doesn't exist.
if (!(-d 'OUTPUT')) {
    mkdir('OUTPUT') or die("Failed to create output folder: $!\n");
}

#Write the configuration file.
open(CONF, '>build.cfg') or die("Failed to write build.cfg: $!\n");
print CONF "OutputBase = " . Build::PathFormat($root . '/OUTPUT') . "\n";
print CONF "SourceBase = $root\n";
if ($^O eq "linux") 
{
    print CONF "BuilderPath = /usr/bin/make\n";
}
else
{
    print CONF "BuilderPath = " . $ENV{'VC9BUILDER'} . "\n";
    print CONF "PDBLog = $root\\OUTPUT\\pdblog.txt\n";
}
close(CONF);

#Do the annoying revision bumping.
#Linux needs some help here.
if ($^O eq "linux")
{
    Build::Command("flip -u modules.versions");
    Build::Command("flip -u tools/versionchanger.pl");
    Build::Command("chmod +x tools/versionchanger.pl");
}
#Build::Command(Build::PathFormat('tools/versionchanger.pl') . ' --buildstring="-dev"');
