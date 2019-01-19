#!/usr/bin/perl

use strict;
use Cwd;
use File::Basename;
use File::stat;
use File::Temp qw/ tempfile :seekable/;
use Net::FTP;
use IO::Uncompress::Gunzip qw(gunzip $GunzipError) ;
use Time::localtime;

my ($ftp_file, $ftp_host, $ftp_user, $ftp_pass, $ftp_path, $tag);

$ftp_file = shift;
$tag = shift;

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
chdir(Build::PathFormat('../../../OUTPUT/package'));

print "Downloading languages.cfg...\n";
# Don't check certificate. It will fail on the slaves and we're resolving to internal addressing anyway
system('wget --no-check-certificate -q -O addons/sourcemod/configs/languages.cfg "https://sm.alliedmods.net/translator/index.php?go=translate&op=export_langs"');
open(my $fh, '<', 'addons/sourcemod/configs/languages.cfg')
    or die "Could not open languages.cfg' $!";
 
while (my $ln = <$fh>) {
    if ($ln =~ /"([^"]+)"\s*"[^"]+.*\((\d+)\) /)
    {
	my $abbr = $1;
	my $id = $2;

	print "Downloading language pack $abbr.zip...\n";
        # Don't check certificate. It will fail on the slaves and we're resolving to internal addressing anyway
        system("wget --no-check-certificate -q -O $abbr.zip \"https://sm.alliedmods.net/translator/index.php?go=translate&op=export&lang_id=$id\"");
        system("unzip -qo $abbr.zip -d addons/sourcemod/translations/");
        unlink("$abbr.zip");
    }
}
close($fh);

my $needNewGeoIP = 1;
if (-e '../GeoIP.dat.gz')
{
	my $stats = stat('../GeoIP.dat.gz');
	if ($stats->size != 0)
	{
		my $fileModifiedTime = $stats->mtime;
		my $fileModifiedMonth = localtime($fileModifiedTime)->mon;
		my $currentMonth = localtime->mon;
		my $thirtyOneDays = 60 * 60 * 24 * 31;

		# GeoIP file only updates once per month
		if ($currentMonth == $fileModifiedMonth || (time() - $fileModifiedTime) < $thirtyOneDays)
		{
			$needNewGeoIP = 0;
		}
	}
}

if ($needNewGeoIP)
{
    print "Downloading GeoIP.dat...\n";
    # Don't check certificate. It will fail on the slaves and we're resolving to internal addressing anyway
    system('wget --no-check-certificate -q -O ../GeoIP.dat.gz https://sm.alliedmods.net/GeoIP.dat.gz');
}
else
{
    print "Reusing existing GeoIP.dat\n";
}

my $geoIPfile = 'addons/sourcemod/configs/geoip/GeoIP.dat';
if (-e $geoIPfile) {
	unlink($geoIPfile);
}

open(my $fh, ">", $geoIPfile) 
    	or die "cannot open $geoIPfile for writing: $!";
binmode($fh);
gunzip '../GeoIP.dat.gz' => $fh
        or die "gunzip failed: $GunzipError\n";
close($fh);

my ($version);

$version = Build::ProductVersion(Build::PathFormat('../../build/product.version'));
$version =~ s/-dev//g;
$version .= '-git' . Build::GitRevNum('../../build');

# Append OS to package version
if ($^O eq "darwin")
{
    $version .= '-mac';
}
elsif ($^O =~ /MSWin/)
{
    $version .= '-windows';
}
else
{
    $version .= '-' . $^O;
}

if (defined $tag)
{
	$version .= '-' . $tag;
}

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

my ($tmpfh, $tmpfile) = tempfile();
print $tmpfh $filename;
$tmpfh->seek( 0, SEEK_END );
my $latest = "sourcemod-latest-";
if ($^O eq "darwin") {
	$latest .= "mac";
}
elsif ($^O =~ /MSWin/) {
	$latest .= "windows";
}
else {
	$latest .= $^O;
}

my ($major,$minor) = ($version =~ /^(\d+)\.(\d+)/);
$ftp_path .= "/$major.$minor";

my ($ftp);

$ftp = Net::FTP->new($ftp_host, Debug => 0, Passive => 1) 
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
$ftp->put($tmpfile, $latest)
    or die "Cannot drop file $latest ($ftp_path): " . $ftp->message . "\n";

$ftp->close();

print "File sent to drop site as $filename -- build succeeded.\n";

exit(0);

