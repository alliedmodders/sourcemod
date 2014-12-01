native void printnum(int num);

methodmap Handle __nullable__
{
	public Handle() {
		return Handle:2;
	}
	public native ~Handle();
};

public main()
{
	Handle egg = new Handle();
	printnum(_:egg);
}
