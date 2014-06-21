native CloseHandle(Handle:this);

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
