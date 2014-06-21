native CloseHandle(Handle:this);

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
