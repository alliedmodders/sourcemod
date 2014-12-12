native printnum(x);

enum X {
};

methodmap X {
	public X(int y) {
		printnum(y);
		return view_as<X>(0);
	}
};

public main()
{
	X x = X(5);
	return X;
}
