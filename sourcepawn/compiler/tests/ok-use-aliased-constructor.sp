native Handle:CreateHandle();

methodmap Handle __nullable__
{
	public native Handle() = CreateHandle;
	public native ~Handle();
};

public main()
{
	CreateHandle();
}
