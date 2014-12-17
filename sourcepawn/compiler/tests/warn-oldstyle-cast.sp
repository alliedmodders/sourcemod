
#pragma newdecls required

enum MyType:{};

native void Print(MyType value);
native void PrintF(float value);

public void main()
{
	int val = 2;
	MyType otherVal = MyType:val;

	float value2 = Float:val;

	Print(otherVal);
	PrintF(value2);
}
