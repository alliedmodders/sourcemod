methodmap Handle {
	public native Handle();
	public native ~Handle();
	public native Handle Clone();
	public native int Size();
	public native float SizeF();
	public native bool Ok(x, y, z);
};

public main() {
	new Handle:handle = Handle();
	handle.Clone();
	new x = handle.Size();
	new Float:f = handle.SizeF();
	new bool:b = handle.Ok(1, 2, 3);
	delete handle;
}
