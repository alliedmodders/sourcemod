native CloseHandle(Handle:handle);

methodmap Handle {
	public Close() = CloseHandle;
};

methodmap Crab < Handle {
};

public main()
{
	new Crab:x;
	x.Close();
}
