#!/usr/bin/perl

use strict;
use Cwd;

package Build;

our $SVN = "/usr/bin/svn";
our $SVN_USER = 'dvander';
our $SVN_ARGS = '';

sub GitRevNum
{
	my ($path) = (@_);
	my ($cd, $text, $rev);

	$cd = Cwd::cwd();
	chdir($path);
	$text = `git rev-list --count HEAD`;
	chdir($cd);

	chomp $text;
	if ($text =~ /^(\d+)/) {
		return $1;
	}

	return 0;
}

sub HgRevNum
{
	my ($path) = (@_);
	my ($cd, $text, $rev);

	$cd = Cwd::cwd();
	chdir($path);
	$text = `hg identify -n`;
	chdir($cd);

	chomp $text;
	if ($text =~ /^(\d+)/)
	{
		return $1;
	}

	return 0;
}

sub SvnRevNum
{
	my ($str)=(@_);
			
	my $data = Command('svnversion -c ' . $str);
	if ($data =~ /(\d+):(\d+)/)
	{
		return $2;
	} elsif ($data =~ /(\d+)/) {
		return $1;
	} else {
		return 0;
	}
}

sub ProductVersion
{
	my ($file) = (@_);
	my ($version);
	open(FILE, $file) or die "Could not open $file: $!\n";
	$version = <FILE>;
	close(FILE);
	chomp $version;
	return $version;
}

sub Delete
{
	my ($str)=(@_);
	if ($^O =~ /MSWin/)
	{
		Command("del /S /F /Q \"$str\"");
		Command("rmdir /S /Q \"$str\"");
	} else {
		Command("rm -rf $str");
	}
	return !(-e $str);
}

sub Copy
{
	my ($src,$dest)=(@_);
	if ($^O =~ /MSWin/)
	{
		Command("copy \"$src\" \"$dest\" /y");
	} else {
		Command("cp \"$src\" \"$dest\"");
	}
	return (-e $dest);
}

sub Move
{
	my ($src,$dest)=(@_);
	if ($^O =~ /MSWin/)
	{
		Command("move \"$src\" \"$dest\"");
	} else {
		Command("mv \"$src\" \"$dest\"");
	}
	return (-e $dest);
}

sub Command
{
	my($cmd)=(@_);
	print "$cmd\n";
	return `$cmd`;
}

sub PathFormat
{
	my ($str)=(@_);
	if ($^O =~ /MSWin/)
	{
		$str =~ s#/#\\#g;
	} else {
		$str =~ s#\\#/#g;
	}
	return $str;
}

sub SVN_Remove
{
	my ($file)=(@_);
	my ($path, $name);
	if ($^O =~ /MSWin/)
	{
		($path, $name) = ($file =~ /(.+)\/([^\/]+)$/);
	} else {
		($path, $name) = ($file =~ /(.+)\\([^\\]+)$/);
	}
	my $dir = Cwd::cwd();
	chdir($path);
	Command($SVN . ' ' . $SVN_ARGS . ' delete ' . $name);
	chdir($dir);
}

sub SVN_Add
{
	my ($file)=(@_);
	my ($path, $name);
	if ($^O =~ /MSWin/)
	{
		($path, $name) = ($file =~ /(.+)\/([^\/]+)$/);
	} else {
		($path, $name) = ($file =~ /(.+)\\([^\\]+)$/);
	}
	my $dir = Cwd::cwd();
	chdir($path);
	Command($SVN . ' ' . $SVN_ARGS . ' add ' . $name);
	chdir($dir);
}

sub GetBuildType
{
	my ($file)=(@_);
	my ($type);
	open(TYPE, $file) or die("Could not open file: $!\n");
	$type = <TYPE>;
	close(TYPE);
	chomp $type;
	return $type;
}

return 1;
