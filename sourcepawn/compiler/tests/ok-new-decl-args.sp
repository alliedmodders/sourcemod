stock A(int n)
{
}

stock B(int n[10])
{
}

stock C(int[] n)
{
}

enum E1: {
}
enum E2 {
}

stock D(E1 t)
{
}

stock E(E2 t)
{
}

stock F(const int n[10]) {
}

stock G(int& n)
{
}

public main()
{
  new x[10]
  new t
  A(1)
  B(x)
  C(x)
  D(E1:5)
  E(E2:5)
  F(x)
  G(t)
}
