native CloseHandle(Handle:this);

methodmap Handle {
	Clone = native Handle:CloneHandle(Handle:this);
	Close = CloseHandle;
};

methodmap Crab < Handle {
};

public main()
{
	new Crab:x;
	x.Close();
}
