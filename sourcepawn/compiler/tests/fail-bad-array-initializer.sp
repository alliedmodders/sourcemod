#define THIS_SIZE_WILL_CRASH 4

new const String:to_crash_compiler[THIS_SIZE_WILL_CRASH][] = 
{
"x",
"x"
};

public main()
{
  return to_crash_compiler[2][1];
}
