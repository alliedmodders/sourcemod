native CloseHandle(Handle:this);

enum Handle {
	INVALID_HANDLE = 0,
};

methodmap Handle {
	Clone = native Handle:CloneHandle(Handle:this);
	Close = CloseHandle;
};

public main()
{
	INVALID_HANDLE.Close();
}
