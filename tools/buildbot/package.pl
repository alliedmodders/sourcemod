#!/usr/bin/perl

use strict;
use Cwd;
use File::Basename;
use Net::FTP;

my ($ftp_file, $ftp_host, $ftp_user, $ftp_pass, $ftp_path);

$ftp_file = shift;

open(FTP, $ftp_file) or die "Unable to read FTP config file $ftp_file: $!\n";
$ftp_host = <FTP>;
$ftp_user = <FTP>;
$ftp_pass = <FTP>;
$ftp_path = <FTP>;
close(FTP);

chomp $ftp_host;
chomp $ftp_user;
chomp $ftp_pass;
chomp $ftp_path;

my ($myself, $path) = fileparse($0);
chdir($path);

require 'helpers.pm';

#Switch to the output folder.
chdir(Build::PathFormat('../../OUTPUT/base'));

my ($version);

$version = Build::ProductVersion(Build::PathFormat('../../product.version'));
$version .= '-hg' . Build::HgRevNum('../..');

my ($filename);
$filename = 'sourcemod-' . $version;
if ($^O eq "linux")
{
    $filename .= '.tar.gz';
    print "tar zcvf $filename addons cfg\n";
    system("tar zcvf $filename addons cfg");
}
else
{
    $filename .= '.zip';
    print "zip -r $filename addons cfg\n";
    system("zip -r $filename addons cfg");
}

my ($major,$minor) = ($version =~ /^(\d+)\.(\d+)/);
$ftp_path .= "/$major.$minor";

my ($ftp);

$ftp = Net::FTP->new($ftp_host, Debug => 0) 
    or die "Cannot connect to host $ftp_host: $@";

$ftp->login($ftp_user, $ftp_pass)
    or die "Cannot connect to host $ftp_host as $ftp_user: " . $ftp->message . "\n";

if ($ftp_path ne '')
{
    $ftp->cwd($ftp_path)
        or die "Cannot change to folder $ftp_path: " . $ftp->message . "\n";
}

$ftp->binary();
$ftp->put($filename)
    or die "Cannot drop file $filename ($ftp_path): " . $ftp->message . "\n";

$ftp->close();

print "File sent to drop site as $filename -- build succeeded.\n";

exit(0);

