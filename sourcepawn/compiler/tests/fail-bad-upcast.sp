native CloseHandle(Handle:this);

methodmap Handle {
	Close = CloseHandle;
};

methodmap Crab {
};

public main()
{
	new Crab:x;
	new Handle:y;
	x = y;
}
