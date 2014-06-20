native CloneHandle(Handle:this);
native CloseHandle(Handle:this);

methodmap Handle {
	public Clone() = CloneHandle;
	public Close() = CloseHandle;
};

public main()
{
	new Handle:x;
	x.Close();
}
