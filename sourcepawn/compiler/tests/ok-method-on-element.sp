native CloseHandle(Handle:handle);

methodmap Handle {
	public Close() = CloseHandle;
};

public main()
{
	new Handle:x[2];
	x[1].Close();
}
