class X
{
};

f({X,Float}:x)
{
	return 3
}

public main()
{
	new X:x;
	return f(x);
}
