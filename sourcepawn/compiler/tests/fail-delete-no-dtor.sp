native Handle:CreateHandle();

methodmap Handle {
	public Handle() = CreateHandle;
};

public main() {
	new Handle:handle = Handle();
	delete handle;
}
