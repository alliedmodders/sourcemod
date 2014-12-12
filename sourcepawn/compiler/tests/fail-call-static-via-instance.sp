native printnum(t);

methodmap X
{
	public static int GetThing() {
		return 10;
	}
}

public main() {
	X x;
	printnum(x.GetThing());
}
