methodmap Handle
{
	public native Handle();
	public native ~Handle();
};

public t()
{
	Handle egg = new Handle();
	delete egg;
}
