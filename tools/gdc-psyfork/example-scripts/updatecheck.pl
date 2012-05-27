#!/usr/bin/perl

use LWP;
use LWP::UserAgent;
use HTTP::Request;

open(STEAMINF, "<", $ARGV[0]);

my $appid = 0;
my $version = "";

while (<STEAMINF>) {
	if ($_ =~ /^(?:Patch)?Version\s*=\s*([0-9\.]+)/) {
		$version = $1;
	} elsif ($_ =~ /appID\s*=\s*([0-9]+)/) {
		$appid = $1;
	}
}
close STEAMINF;

if ($appid == 0 || $version eq "") {
	print "Failed to parse steam.inf\n";
	exit 2;
}
 
my $request = new HTTP::Request(
	'GET',
	sprintf("http://api.steampowered.com/ISteamApps/UpToDateCheck/v0001/?appid=%d&version=%s&format=xml",
		$appid,
		$version
		)
	);

my $ua = new LWP::UserAgent;
my $response = $ua->request($request);

if (!$response->is_success())
{
	printf("Bad response code: %d\n", $response->code());
	exit 3;
}

$version =~ s/\.//g;

if ($response->content() =~ /<up_to_date>\s*false\s*<\/up_to_date>/) {
	printf("We are outdated (v%s)\n", $version);
	if ($response->content() =~ /<required_version>\s*([0-9\.]+)\s*<\/required_version>/) {
		my $newversion = $1;
		printf("Latest is v%s\n", $newversion);
		open(my $f, ">", $ARGV[0].".new");
		print $f $newversion;
	}
	exit 0;
} else {
	printf("We are up to date (v%s)\n", $version);
	exit 1;
}
