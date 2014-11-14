methodmap Egg
{
  public void illegal(Egg x) {
    this = x;
  }
};

public main()
{
  Egg egg;
  egg.illegal(egg);
}
