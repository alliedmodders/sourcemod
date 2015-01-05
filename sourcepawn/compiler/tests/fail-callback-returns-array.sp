String:MyFunction()
{
  new String:egg[10] = "egg";
  return egg;
}

public crab()
{
  new String:egg[10];
  egg = MyFunction();
}

public Function:main()
{
  return MyFunction;
}
