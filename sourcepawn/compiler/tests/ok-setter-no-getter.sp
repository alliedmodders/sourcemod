methodmap Crab
{
	property int X {
		public native set(int value);
	}
}

public main()
{
	Crab crab;
	crab.X = 12;
}
