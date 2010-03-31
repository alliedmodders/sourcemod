use File::Glob;

my $output = <<EOF;
<?php
/**
 * =============================================================================
 * SourcePawn GeSHi Syntax File
 * Copyright (C) 2010 AlliedModders LLC
 * INC parser originally by Zach "theY4Kman" Kanzler,
 *     ported to perl and enhanced by Nicholas "psychonic" Hastings
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

\$language_data = array(
	"LANG_NAME" => "SourcePawn",
	"COMMENT_SINGLE" => array(1 => "//", 2 => "#"),
	"COMMENT_MULTI" => array("/*" => "*/"),
	"CASE_KEYWORDS" => GESHI_CAPS_NO_CHANGE,
	"QUOTEMARKS" => array('"'),
	"ESCAPE_CHAR" => "\\\\",
	"SYMBOLS" => array(
		0 => array(";"),
		// Assignment operators
		1 => array("=", "+=", "-=", "/=", "*=", "&=", "|=", "~=", "^="),
		// Comparison and logical operators
		2 => array("==", "!=", "&&", "||", ">", "<", "<=", ">="),
		// Other operators
		3 => array("+", "-", "*", "/", "|", "&", "~", "++", "--", "^", "%%", "!"),
		),
	"KEYWORDS" => array(
		// Reserved words
		1 => array(
			"for", "if", "else", "do", "while", "switch", "case", "return",
			"break", "continue", "new", "decl", "public", "stock", "const",
			"enum", "forward", "static", "funcenum", "functag", "native"
			),
		// Tags
		2 => array(
			"Action", "bool", "Float", "Handle", "Plugin", "String", "any",
			__tags__
			),
		// Natives
		3 => array(
			__natives__
			),
		// Forwards
		4 => array(
			__forwards__
			),
		// Defines
		5 => array(
			"MaxClients",
			__defines__
			),
		),
	"REGEXPS" => array(),
	"STYLES" => array(
        "KEYWORDS" => array(
            1 => "color: #0000EE; font-weight: bold;",
            2 => "color: #218087; font-weight: bold;",
            3 => "color: #000000; font-weight: bold;",
            4 => "color: #000000; font-weight: bold;",
            5 => "color: #8000FF;",
            ),
        "COMMENTS" => array(
            1 => "color: #006600; font-style: italic;",
            2 => "color: #0000aa;",
            "MULTI" => "color: #006600; font-style: italic;",
            ),
        "ESCAPE_CHAR" => array(
            0 => "color: #ff00ff;",
            ),
        "SYMBOLS" => array(
            0 => "color: #000000; font-weight: bold;",
            1 => "color: #880000;",
            2 => "color: #886600;",
            3 => "color: #AA8800;",
            ),
        "STRINGS" => array(
            0 => "color: #B90000;",
            ),
        "BRACKETS" => array(
            0 => "color: #33aa33; font-weight: bold;",
            ),
        "NUMBERS" => array(
            0 => "color: #AE5700;",
            ),
        ),
);

?>
EOF

my @incs = glob("sourcepawn_include/*.inc");
my %generated_stuff = {};
my @forwards = ();
my @natives = ();
my @tags = ();
my @defines = ();

foreach (@incs)
{
	undef $/;
	open FILE, $_ or die "Couldn't open file: $!";
	binmode FILE;
	my $contents = <FILE>;
	close FILE;
	
	push(@tags, $contents =~ m/enum\s+([a-zA-Z][a-zA-Z0-9_-]*)\s*\{/g);
	push(@forwards, $contents =~ m/forward\s+(?:[a-zA-Z]*:)?([a-zA-Z][a-zA-Z0-9_-]*)\s*\(/g);
	push(@natives, $contents =~ m/native\s+(?:[a-zA-Z]*:)?([a-zA-Z][a-zA-Z0-9_-]*)\s*\(/g);
	push(@natives, $contents =~ m/stock\s+(?:[a-zA-Z]*:)?([a-zA-Z][a-zA-Z0-9_-]*)\s*\(/g);
	push(@defines, $contents =~ m/\#define\s+([^_\s][^\s]*)/g);
	while ($contents =~ m/[^c]enum\s+(?:\w+\s+)?{(.+?)}/sg)
	{
		my $enumcontents = $1;
		$enumcontents =~ s/=\s*[a-zA-Z0-9-_]+\s*//g;
		$enumcontents =~ s/\/\/.*//g;
		$enumcontents =~ s/\/\*.*?\*\///sg;
		$enumcontents =~ s/\s//g;
		$enumcontents =~ s/,$//;
		push(@defines, split(/,/, $enumcontents));
	}
}

foreach (@tags)
{
	$generated_stuff{tags} .= "\"$_\",";
}

foreach (@forwards)
{
	$generated_stuff{forwards} .= "\"$_\",";
}

foreach (@natives)
{
	$generated_stuff{natives} .= "\"$_\",";
}

foreach (@defines)
{
	$generated_stuff{defines} .= "\"$_\",";
}

$output =~ s/__tags__/$generated_stuff{tags}/;
$output =~ s/__forwards__/$generated_stuff{forwards}/;
$output =~ s/__natives__/$generated_stuff{natives}/;
$output =~ s/__defines__/$generated_stuff{defines}/;

open OUTPUTTEXT, ">sourcepawn.php";
print OUTPUTTEXT $output;
close OUTPUTTEXT;