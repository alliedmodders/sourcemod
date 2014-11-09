native CloseHandle(Handle:handle);

methodmap Handle {
	public Close() = CloseHandle;
};

public main()
{
	main.Close();
}
