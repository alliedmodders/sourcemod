enum Action: {}
functag public Action:OldFuncTag( String:someString[128] );
typedef NewFuncTag = function Action ( char someString[128] );

native UseOldFuncTag( OldFuncTag func );
native UseNewFuncTag( NewFuncTag func );

public OnPluginStart()
{
	// fine
	UseOldFuncTag( MyOldFunc );
	// also fine
	UseOldFuncTag( MyNewFunc );

	// error 100: function prototypes do not match
	UseNewFuncTag( MyOldFunc );
	// error 100: function prototypes do not match
	UseNewFuncTag( MyNewFunc );
}

public Action:MyOldFunc( String:someString[128] )
{
}
public Action MyNewFunc( char someString[128] )
{
}
