native CloseHandle(Handle:this);

methodmap Handle {
	public Close() = CloseHandle;
};

public main()
{
	main.Close();
}
