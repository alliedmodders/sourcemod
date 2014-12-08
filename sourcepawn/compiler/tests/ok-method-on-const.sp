native CloseHandle(Handle:handle);

enum Handle {
	INVALID_HANDLE = 0,
};

methodmap Handle {
	public Close() = CloseHandle;
};

public main()
{
	INVALID_HANDLE.Close();
}
