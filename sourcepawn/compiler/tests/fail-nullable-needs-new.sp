methodmap Handle __nullable__
{
	public native Handle();
	public native ~Handle();
};

public t()
{
	Handle egg = Handle();
	delete egg;
}
