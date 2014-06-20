native CloseHandle(Handle:this);

methodmap Handle {
	public Close() = CloseHandle;
};

public main()
{
	new Handle:x[2];

	x.Close();
}
