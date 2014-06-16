native CloseHandle(Handle:this);

methodmap Handle {
	Clone = native Handle:CloneHandle(Handle:this);
	Close = CloseHandle;
};

f(const &Handle:x)
{
	x.Close()
}

public main()
{
	new Handle:x;
	f(x)
}
