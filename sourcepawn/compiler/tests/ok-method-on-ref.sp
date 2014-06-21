native CloseHandle(Handle:this);

methodmap Handle {
	public Close() = CloseHandle;
};

f(&Handle:x)
{
	x.Close()
}

public main()
{
	new Handle:x;
	f(x)
}
