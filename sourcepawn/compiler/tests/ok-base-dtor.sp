native Handle:CreateHandle(count);
native CloseHandle(Handle:handle);

methodmap Handle {
	public Handle() = CreateHandle;
	public ~Handle() = CloseHandle;
};

methodmap Crab < Handle {
};

public main() {
	new Crab:crab;
	delete crab;
}
