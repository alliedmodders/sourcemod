
new String:oldArray[][] =
{
	"string",
	"string2",
};

char newArray[][] =
{
	"another string",
	"more strings",
};

native Print( const String:string[] );

public OnPluginStart()
{
	Print( oldArray[ 0 ] );
	Print( newArray[ 0 ] );
}
