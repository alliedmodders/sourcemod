native CloseHandle(Handle:handle);

methodmap Handle {
};

methodmap Crab < Handle {
	public Close() = CloseHandle;
};

public main()
{
	new Crab:x;
	x.Close();
}
