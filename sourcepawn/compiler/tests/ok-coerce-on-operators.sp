enum Handle {
	INVALID_HANDLE,
};

methodmap Handle {};
methodmap ArrayList < Handle {};

public void MyCommand()
{
    ArrayList myList;
    if (INVALID_HANDLE == myList)
    {
    }
    if (myList == INVALID_HANDLE)
    {
    }
}

