methodmap Handle __nullable__
{
	public native Handle();
	public native ~Handle();
};

enum Crab {};

public t()
{
	Crab egg = new Crab();
	delete egg;
}
