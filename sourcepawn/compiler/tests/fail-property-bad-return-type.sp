native GetCrab(Crab:crab);

methodmap Crab {
	property float Blah {
		public get() = GetCrab;
	}
}

public main() {
	new Crab:crab = Crab:5;
	new x = crab.Blah;
}
