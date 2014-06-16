native CloseHandle(HandleEgg:this);

methodmap Handle {
	Clone = native Handle:CloneHandle(Handle:this);
	Close = CloseHandle;
};

public main()
{
}
