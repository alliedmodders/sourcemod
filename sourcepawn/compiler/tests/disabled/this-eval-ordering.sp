native void printnum(int x);

methodmap X
{
  public void GetValue() {
    printnum(view_as<int>(this));
  }
  public void SetValue(const char[] key, any value) {
    printnum(view_as<int>(this));
  }
};

int Sandwich(const char[] value)
{
  return 999;
}

//////////////////////////////////

X gTargets[] = {
  view_as<X>(10),
  view_as<X>(20),
  view_as<X>(30),
}

// Should print 30 three times.
public main()
{
  printnum(view_as<int>(gTargets[2]));
  gTargets[2].GetValue();
  gTargets[2].SetValue("TargetBanTime", Sandwich("hello"));
}
