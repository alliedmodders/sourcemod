native CloseHandle(Handle:this);

methodmap Handle {
	public Close() = CloseHandle;
};

methodmap Crab {
};

public main()
{
	new Crab:x;
	new Handle:y;
	x = y;
}
