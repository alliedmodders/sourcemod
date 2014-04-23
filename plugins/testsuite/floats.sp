public OnPluginStart()
{
  RegServerCmd("test_floats", TestFloats)
}

check(bool:got, bool:expect, const String:message[]="")
{
  if (got != expect) {
    ThrowError("Check failed", message)
  }
}

public Action:TestFloats(args)
{
  new Float:x = 5.3
  new Float:y = 10.2

  check(x < y, true)
  check(x <= y, true)
  check(x > y, false)
  check(x >= y, false)
  check(x == y, false)
  check(x != y, true)

  x = 10.5
  y = 2.3

  check(x < y, false)
  check(x <= y, false)
  check(x > y, true)
  check(x >= y, true)
  check(x == y, false)
  check(x != y, true)

  x = 10.5
  y = x

  check(x < y, false)
  check(x <= y, true)
  check(x > y, false)
  check(x >= y, true)
  check(x == y, true)
  check(x != y, false)

  x = 0.0
  y = 0.0
  new Float:nan = x / y

  check(x < nan, false)
  check(x <= nan, false)
  check(x > nan, false)
  check(x >= nan, false)
  check(x == nan, false)
  check(x != nan, true)
  check(nan < y, false)
  check(nan <= y, false)
  check(nan > y, false)
  check(nan >= y, false)
  check(nan == y, false)
  check(nan != y, true)
  check(nan == nan, false)
  check(nan != nan, true)

  x = 10.5
  y = 0.0
  check(!x, false)
  check(!y, true)
  check(!nan, true)

  y = -2.7
  check(-x == -10.5, true)
  check(-y == 2.7, true)

  new String:buffer[32]
  Format(buffer, sizeof(buffer), "%f", nan)
  check(StrEqual(buffer, "NaN"), true)

  PrintToServer("Tests finished.")

  return Plugin_Stop
}
