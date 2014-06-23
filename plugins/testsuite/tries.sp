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
  RegServerCmd("test_tries", RunTests);
}

public Action:RunTests(argc)
{
  new Handle:trie = CreateTrie();

  for (new i = 0; i < 64; i++) {
    new String:buffer[24];
    Format(buffer, sizeof(buffer), "%d", i);

    if (!SetTrieValue(trie, buffer, i))
      ThrowError("set trie to %d failed", i);

    new value;
    if (!GetTrieValue(trie, buffer, value))
      ThrowError("get trie %d", i);
    if (value != i)
      ThrowError("get trie %d == %d", i, i);
  }

  // Setting 17 without replace should fail.
  new value;
  if (SetTrieValue(trie, "17", 999, false))
    ThrowError("set trie 17 should fail");
  if (!GetTrieValue(trie, "17", value) || value != 17)
    ThrowError("value at 17 not correct");
  if (!SetTrieValue(trie, "17", 999))
    ThrowError("set trie 17 = 999 should succeed");
  if (!GetTrieValue(trie, "17", value) || value != 999)
    ThrowError("value at 17 not correct");

  // Check size is 64.
  if (GetTrieSize(trie) != 64)
    ThrowError("trie size not 64");

  // Check "cat" is not found.
  new array[64];
  new String:string[64];
  if (GetTrieValue(trie, "cat", value) ||
      GetTrieArray(trie, "cat", array, sizeof(array)) ||
      GetTrieString(trie, "cat", string, sizeof(string)))
  {
    ThrowError("trie should not have a cat");
  }

  // Check that "17" is not a string or array.
  if (GetTrieArray(trie, "17", array, sizeof(array)) ||
      GetTrieString(trie, "17", string, sizeof(string)))
  {
    ThrowError("entry 17 should not be an array or string");
  }

  // Strings.
  if (!SetTrieString(trie, "17", "hellokitty"))
    ThrowError("17 should be string");
  if (!GetTrieString(trie, "17", string, sizeof(string)) ||
      strcmp(string, "hellokitty") != 0)
  {
    ThrowError("17 should be hellokitty");
  }
  if (GetTrieValue(trie, "17", value) ||
      GetTrieArray(trie, "17", array, sizeof(array)))
  {
    ThrowError("entry 17 should not be an array or string");
  }

  // Arrays.
  new data[5] = { 93, 1, 2, 3, 4 };
  if (!SetTrieArray(trie, "17", data, 5))
    ThrowError("17 should be string");
  if (!GetTrieArray(trie, "17", array, sizeof(array)))
    ThrowError("17 should be hellokitty");
  for (new i = 0; i < 5; i++) {
    if (data[i] != array[i])
      ThrowError("17 slot %d should be %d, got %d", i, data[i], array[i]);
  }
  if (GetTrieValue(trie, "17", value) ||
      GetTrieString(trie, "17", string, sizeof(string)))
  {
    ThrowError("entry 17 should not be an array or string");
  }

  if (!SetTrieArray(trie, "17", data, 1))
    ThrowError("couldn't set 17 to 1-entry array");
  // Check that we fixed an old bug where 1-entry arrays where cells
  if (!GetTrieArray(trie, "17", array, sizeof(array), value))
    ThrowError("couldn't fetch 1-entry array");
  if (value != 1)
    ThrowError("array size mismatch (%d, expected %d)", value, 1);
  // Check that we maintained backward compatibility.
  if (!GetTrieValue(trie, "17", value))
    ThrowError("backwards compatibility failed");
  if (value != data[0])
    ThrowError("wrong value (%d, expected %d)", value, data[0]);

  // Remove "17".
  if (!RemoveFromTrie(trie, "17"))
    ThrowError("17 should have been removed");
  if (RemoveFromTrie(trie, "17"))
    ThrowError("17 should not exist");
  if (GetTrieValue(trie, "17", value) ||
      GetTrieArray(trie, "17", array, sizeof(array)) ||
      GetTrieString(trie, "17", string, sizeof(string)))
  {
    ThrowError("trie should not have a 17");
  }

  ClearTrie(trie);

  if (GetTrieSize(trie))
    ThrowError("size should be 0");

  PrintToServer("All tests passed!");
  CloseHandle(trie);
  return Plugin_Handled;
}

