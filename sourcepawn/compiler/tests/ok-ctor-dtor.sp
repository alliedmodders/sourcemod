native Handle:CreateHandle(count);
native CloseHandle(Handle:handle);

methodmap Handle {
	public Handle = CreateHandle;
	public ~Handle = CloseHandle;
	public Close = CloseHandle;
};

public main() {
	new Handle:handle = Handle(3);
	handle.Close();
}
