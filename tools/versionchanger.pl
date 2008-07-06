#!/usr/bin/perl

our %arguments =
(
	'config' => 'modules.versions',
	'major' => '1',
	'minor' => '0',
	'revision' => '0',
	'build' => undef,
	'svnrev' => 'global',
	'path' => '',
);

my $arg;
foreach $arg (@ARGV)
{
	$arg =~ s/--//;
	@arg = split(/=/, $arg);
	$arguments{$arg[0]} = $arg[1];
}

#Set up path info
if ($arguments{'path'} ne "")
{
	if (!(-d $arguments{'path'}))
	{
		die "Unable to find path: " . $arguments{'path'} ."\n";
	}
	chdir($arguments{'path'});
}

if (!open(CONFIG, $arguments{'config'}))
{
	die "Unable to open config file for reading: " . $arguments{'config'} . "\n";
}

our %modules;
my $cur_module = undef;
my $line;
while (<CONFIG>)
{
	chomp;
	$line = $_;
	if ($line =~ /^\[([^\]]+)\]$/)
	{
		$cur_module = $1;
		next;
	}
	if (!$cur_module)
	{
		next;
	}
	if ($line =~ /^([^=]+) = (.+)$/)
	{
		$modules{$cur_module}{$1} = $2;
	}
}

close(CONFIG);

#Copy global configuration options...
if (exists($modules{'PRODUCT'}))
{
	if (exists($modules{'PRODUCT'}{'major'}))
	{
		$arguments{'major'} = $modules{'PRODUCT'}{'major'};
	}
	if (exists($modules{'PRODUCT'}{'minor'}))
	{
		$arguments{'minor'} = $modules{'PRODUCT'}{'minor'};
	}
	if (exists($modules{'PRODUCT'}{'revision'}))
	{
		$arguments{'revision'} = $modules{'PRODUCT'}{'revision'};
	}
	if (exists($modules{'PRODUCT'}{'svnrev'}))
	{
		$arguments{'svnrev'} = $modules{'PRODUCT'}{'svnrev'};
	}
}

#Get the global SVN revision if we have none
my $rev;
if ($arguments{'build'} == undef)
{
	$rev = GetRevision(undef);
} else {
	$rev = int($arguments{'build'});
}

my $major = $arguments{'major'};
my $minor = $arguments{'minor'};
my $revision = $arguments{'revision'};
my $svnrev = $arguments{'svnrev'};

#Go through everything now
my $mod_i;
while ( ($cur_module, $mod_i) = each(%modules) )
{
	#Skip the magic one
	if ($cur_module eq "PRODUCT")
	{
		next;
	}
	#Prepare path
	my %mod = %{$mod_i};
	my $infile = $mod{'in'};
	my $outfile = $mod{'out'};
	if ($mod{'folder'})
	{
		if (!(-d $mod{'folder'}))
		{
			die "Folder " . $mod{'folder'} . " not found.\n";
		}
		$infile = $mod{'folder'} . '/' . $infile;
		$outfile = $mod{'folder'} . '/' . $outfile;
	}
	if (!(-f $infile))
	{
		die "File $infile is not a file.\n";
	}
	my $global_rev = $rev;
	my $local_rev = GetRevision($mod{'folder'});
	if ($arguments{'svnrev'} eq 'local')
	{
		$global_rev = $local_rev;
	}
	#Start rewriting
	open(INFILE, $infile) or die "Could not open file for reading: $infile\n";
	open(OUTFILE, '>'.$outfile) or die "Could not open file for writing: $outfile\n";
	while (<INFILE>)
	{
		s/\$PMAJOR\$/$major/g;
		s/\$PMINOR\$/$minor/g;
		s/\$PREVISION\$/$revision/g;
		s/\$GLOBAL_BUILD\$/$rev/g;
		s/\$LOCAL_BUILD\$/$local_rev/g;
		print OUTFILE $_;
	}
	close(OUTFILE);
	close(INFILE);
}

sub GetRevision
{
	my ($path)=(@_);
	my $rev;
	if (!$path)
	{
		$rev = `svnversion --committed`;
	} else {
		$rev = `svnversion --committed $path`;
	}
	if ($rev =~ /exported/)
	{
		die "Path specified is not a working copy\n";
	} elsif ($rev =~ /(\d+):(\d+)/) {
		$rev = int($2);
	} elsif ($rev =~ /(\d+)/) {
		$rev = int($1);
	} else {
		die "Unknown svnversion response: $rev\n";
	}
	return $rev;
}
