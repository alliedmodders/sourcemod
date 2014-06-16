native CloseHandle(Handle:this);

methodmap Handle {
	Clone = native Handle:CloneHandle(Handle:this);
	Close = CloseHandle;
};

public main()
{
	new Handle:x[2];
	x[1].Close();
}
