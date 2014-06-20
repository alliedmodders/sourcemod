native Float:CreateHandle(count);
native CloseHandle(Handle:handle);

methodmap Handle {
	public Handle() = CreateHandle;
	public Close() = CloseHandle;
};

public main() {
	new Handle:handle = Handle(3);
	handle.Close();
}

