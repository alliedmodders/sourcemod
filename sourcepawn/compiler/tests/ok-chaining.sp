methodmap Duck
{
	property bool MyProp
	{
		public get() {
			return true;
		}
	}
};

public bool OnPluginStart()
{
	Duck duck = GiveMeADuck();

	// no compile errors or warnings
	if (duck.MyProp)
	{
	}

	// error 001: expected token: ")", but found "."
	// error 029: invalid expression, assumed zero
	// error 017: undefined symbol "MyProp"
	if (GiveMeADuck().MyProp)
	{
	}

	// warning 213: tag mismatch
	// error 001: expected token: ";", but found "."
	// error 029: invalid expression, assumed zero
	// error 017: undefined symbol "MyProp"
	bool prop = GiveMeADuck().MyProp;
	return prop
}

stock Duck GiveMeADuck()
{
	return Duck:1;
}
