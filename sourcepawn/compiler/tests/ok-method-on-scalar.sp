native CloneHandle(Handle:handle);
native CloseHandle(Handle:handle);

methodmap Handle {
	public Clone() = CloneHandle;
	public Close() = CloseHandle;
};

public main()
{
	new Handle:x;
	x.Close();
}
