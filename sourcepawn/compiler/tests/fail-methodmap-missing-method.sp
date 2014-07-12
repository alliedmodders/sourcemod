methodmap Duck
{
	public void MyFunc()
	{
		// this will compile fine until this function is used elsewhere in code
		ThisFuncDoesntExist();
	}
};

public OnPluginStart()
{
}

