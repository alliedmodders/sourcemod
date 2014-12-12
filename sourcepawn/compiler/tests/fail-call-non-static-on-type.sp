native printnum(t);

methodmap X
{
	public int GetThing() {
		return 10;
	}
}

public main() {
	printnum(X.GetThing());
}
