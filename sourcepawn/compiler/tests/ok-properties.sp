native Float:GetCrabWhat(Crab:crab);

methodmap Crab {
	public Crab(int n) {
		return Crab:n;
	}
	property Crab Yams {
		public get() {
			return Crab:5;
		}
	}
	property int Blah {
		public native get();
	}
	property float What {
		public get() = GetCrabWhat;
	}
}

print(n) {
	return n
}

public main() {
	new Crab:crab = Crab(10);
	print(_:crab.Yams.Yams.Yams)
	print(crab.Blah);
	print(_:crab.What);
}
