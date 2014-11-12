public Action:SomeEvent( Handle:event, const String:name[], bool:dontBroadcast)
{
    // error 143: new-style declarations should not have "new"
    new object = GetEventInt(event, "object");
}
