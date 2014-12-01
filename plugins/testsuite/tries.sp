// vim: set sts=2 ts=8 sw=2 tw=99 et ft=c :
#include <sourcemod>

public Plugin:myinfo = 
{
  name        = "Trie test",
  author      = "AlliedModders LLC",
  description = "Trie tests",
  version     = "1.0.0.0",
  url         = "http://www.sourcemod.net/"
};

public OnPluginStart()
{
  RegServerCmd("test_maps", RunTests);
}

public Action:RunTests(argc)
{
  StringMap map = new StringMap();

  for (new i = 0; i < 64; i++) {
    char buffer[24];
    Format(buffer, sizeof(buffer), "%d", i);

    if (!map.SetValue(buffer, i))
      ThrowError("set map to %d failed", i);

    new value;
    if (!map.GetValue(buffer, value))
      ThrowError("get map %d", i);
    if (value != i)
      ThrowError("get map %d == %d", i, i);
  }

  // Setting 17 without replace should fail.
  new value;
  if (map.SetValue("17", 999, false))
    ThrowError("set map 17 should fail");
  if (!map.GetValue("17", value) || value != 17)
    ThrowError("value at 17 not correct");
  if (!map.SetValue("17", 999))
    ThrowError("set map 17 = 999 should succeed");
  if (!map.GetValue("17", value) || value != 999)
    ThrowError("value at 17 not correct");

  // Check size is 64.
  if (map.Size != 64)
    ThrowError("map size not 64");

  // Check "cat" is not found.
  int array[64];
  char string[64];
  if (map.GetValue("cat", value) ||
      map.GetArray("cat", array, sizeof(array)) ||
      map.GetString("cat", string, sizeof(string)))
  {
    ThrowError("map should not have a cat");
  }

  // Check that "17" is not a string or array.
  if (map.GetArray("17", array, sizeof(array)) ||
      map.GetString("17", string, sizeof(string)))
  {
    ThrowError("entry 17 should not be an array or string");
  }

  // Strings.
  if (!map.SetString("17", "hellokitty"))
    ThrowError("17 should be string");
  if (!map.GetString("17", string, sizeof(string)) ||
      strcmp(string, "hellokitty") != 0)
  {
    ThrowError("17 should be hellokitty");
  }
  if (map.GetValue("17", value) ||
      map.GetArray("17", array, sizeof(array)))
  {
    ThrowError("entry 17 should not be an array or string");
  }

  // Arrays.
  new data[5] = { 93, 1, 2, 3, 4 };
  if (!map.SetArray("17", data, 5))
    ThrowError("17 should be string");
  if (!map.GetArray("17", array, sizeof(array)))
    ThrowError("17 should be hellokitty");
  for (new i = 0; i < 5; i++) {
    if (data[i] != array[i])
      ThrowError("17 slot %d should be %d, got %d", i, data[i], array[i]);
  }
  if (map.GetValue("17", value) ||
      map.GetString("17", string, sizeof(string)))
  {
    ThrowError("entry 17 should not be an array or string");
  }

  if (!map.SetArray("17", data, 1))
    ThrowError("couldn't set 17 to 1-entry array");
  // Check that we fixed an old bug where 1-entry arrays where cells
  if (!map.GetArray("17", array, sizeof(array), value))
    ThrowError("couldn't fetch 1-entry array");
  if (value != 1)
    ThrowError("array size mismatch (%d, expected %d)", value, 1);
  // Check that we maintained backward compatibility.
  if (!map.GetValue("17", value))
    ThrowError("backwards compatibility failed");
  if (value != data[0])
    ThrowError("wrong value (%d, expected %d)", value, data[0]);

  // Remove "17".
  if (!map.Remove("17"))
    ThrowError("17 should have been removed");
  if (map.Remove("17"))
    ThrowError("17 should not exist");
  if (map.GetValue("17", value) ||
      map.GetArray("17", array, sizeof(array)) ||
      map.GetString("17", string, sizeof(string)))
  {
    ThrowError("map should not have a 17");
  }

  map.Clear();

  if (map.Size)
    ThrowError("size should be 0");

  map.SetString("adventure", "time!");
  map.SetString("butterflies", "bees");
  map.SetString("egg", "egg");

  StringMapSnapshot keys = map.Snapshot();
  {
    if (keys.Length != 3)
      ThrowError("map snapshot length should be 3");

    bool found[3];
    for (new i = 0; i < keys.Length; i++) {
      new size = keys.KeyBufferSize(i);
      char[] buffer = new char[size];
      keys.GetKey(i, buffer, size);

      if (strcmp(buffer, "adventure") == 0)
        found[0] = true;
      else if (strcmp(buffer, "butterflies") == 0)
        found[1] = true;
      else if (strcmp(buffer, "egg") == 0)
        found[2] = true;
      else
        ThrowError("unexpected key: %s", buffer);
    }

    if (!found[0] || !found[1] || !found[2])
      ThrowError("did not find all keys");
  }
  delete keys;

  PrintToServer("All tests passed!");

  delete map;
  return Plugin_Handled;
}

