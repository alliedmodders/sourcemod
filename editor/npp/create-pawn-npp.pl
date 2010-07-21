use File::Glob;

my $output = <<EOF;
<NotepadPlus>
    <UserLang name="sourcemod" ext="sma inc p pawn core sp">
        <Settings>
            <Global caseIgnored="no" escapeChar="\\" />
            <TreatAsSymbol comment="yes" commentLine="yes" />
            <Prefix words1="no" words2="yes" words3="no" words4="no" />
        </Settings>
        <KeywordLists>
            <Keywords name="Delimiters">&quot;&apos;0&quot;&apos;0</Keywords>
            <Keywords name="Folder+">{</Keywords>
            <Keywords name="Folder-">}</Keywords>
            <Keywords name="Operators">+ - * / = ! % &amp; ( ) , . : ; ? @ [ ] ^ | ~ + &lt; = &gt;</Keywords>
            <Keywords name="Comment">1/* 2*/ 0//</Keywords>
            <Keywords name="Words1">for if else do while switch case default return break continue new decl public stock const enum forward static funcenum functag native sizeof true false</Keywords>
            <Keywords name="Words2">#</Keywords>
            <Keywords name="Words3">Action bool Float Plugin String any __tags__</Keywords>
            <Keywords name="Words4">MaxClients __defines__</Keywords>
        </KeywordLists>
        <Styles>
            <WordsStyle name="DEFAULT" styleID="11" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" />
            <WordsStyle name="FOLDEROPEN" styleID="12" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" />
            <WordsStyle name="FOLDERCLOSE" styleID="13" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" />
            <WordsStyle name="KEYWORD1" styleID="5" fgColor="0000FF" bgColor="FFFFFF" fontName="" fontStyle="0" />
            <WordsStyle name="KEYWORD2" styleID="6" fgColor="0000A0" bgColor="FFFFFF" fontName="" fontStyle="0" />
            <WordsStyle name="KEYWORD3" styleID="7" fgColor="25929A" bgColor="FFFFFF" fontName="" fontStyle="0" />
            <WordsStyle name="KEYWORD4" styleID="8" fgColor="8000FF" bgColor="FFFFFF" fontName="" fontStyle="0" />
            <WordsStyle name="COMMENT" styleID="1" fgColor="008040" bgColor="FFFFFF" fontName="" fontStyle="0" />
            <WordsStyle name="COMMENT LINE" styleID="2" fgColor="008040" bgColor="FFFFFF" fontName="" fontStyle="0" />
            <WordsStyle name="NUMBER" styleID="4" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" />
            <WordsStyle name="OPERATOR" styleID="10" fgColor="000000" bgColor="FFFFFF" fontName="" fontStyle="0" />
            <WordsStyle name="DELIMINER1" styleID="14" fgColor="B90000" bgColor="FFFFFF" fontName="" fontStyle="0" />
            <WordsStyle name="DELIMINER2" styleID="15" fgColor="B90000" bgColor="FFFFFF" fontName="" fontStyle="0" />
            <WordsStyle name="DELIMINER3" styleID="16" fgColor="000000" bgColor="112435" fontName="" fontStyle="0" />
        </Styles>
    </UserLang>
</NotepadPlus>
EOF

my @incs = glob("sourcepawn_include/*.inc");
my %generated_stuff = {};
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
	push(@defines, $contents =~ m/\#define\s+([^_\s][^\s]*)/g);
	while ($contents =~ m/[^c]enum\s+(?:\w+\s+)?{(.+?)}/sg)
	{
		my $enumcontents = $1;
		$enumcontents =~ s/=\s*[a-zA-Z0-9\+\-\*\/_\|&><\(\)~\^!=]+\s*//g;
		$enumcontents =~ s/\/\/.*//g;
		$enumcontents =~ s/\/\*.*?\*\///sg;
		$enumcontents =~ s/\s//g;
		$enumcontents =~ s/,$//;
		push(@defines, split(/,/, $enumcontents));
	}
}

foreach (@tags)
{
	$generated_stuff{tags} .= "$_ ";
}

foreach (@defines)
{
	$generated_stuff{defines} .= "$_ ";
}

$output =~ s/__tags__/$generated_stuff{tags}/;
$output =~ s/__defines__/$generated_stuff{defines}/;

open OUTPUTTEXT, ">userDefineLang.xml";
print OUTPUTTEXT $output;
close OUTPUTTEXT;