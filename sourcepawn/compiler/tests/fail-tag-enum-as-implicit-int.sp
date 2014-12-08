native Float:float(x);
native Float:FloatMul(Float:oper1, Float:oper2);

stock Float:operator*(Float:oper1, oper2)
{
  return FloatMul(oper1, float(oper2));
}

native Float:GetRandomFloat();

enum _:Rocketeer {
  bool:bActivated,
  iRockets[20]
};

public OnPluginStart()
{
  decl Float:fAngles[3];

  fAngles[0] = GetRandomFloat() * 360;
  fAngles[1] = GetRandomFloat() * 360;
  fAngles[2] = GetRandomFloat() * 360;
}
