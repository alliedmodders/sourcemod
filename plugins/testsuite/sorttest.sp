#include <sourcemod>

public Plugin:myinfo = 
{
	name = "Sorting Test",
	author = "AlliedModders LLC",
	description = "Tests sorting functions",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

public OnPluginStart()
{
	RegServerCmd("test_sort_ints", Command_TestSortInts)
	RegServerCmd("test_sort_floats", Command_TestSortFloats)
	RegServerCmd("test_sort_strings", Command_TestSortStrings)
	RegServerCmd("test_sort_1d", Command_TestSort1D)
	RegServerCmd("test_sort_2d", Command_TestSort2D)
	RegServerCmd("test_adtsort_ints", Command_TestSortADTInts)
	RegServerCmd("test_adtsort_floats", Command_TestSortADTFloats)
	RegServerCmd("test_adtsort_strings", Command_TestSortADTStrings)
	RegServerCmd("test_adtsort_custom", Command_TestSortADTCustom)
}

/*****************
 * INTEGER TESTS *
 *****************/
// Note that integer comparison is just int1-int2 (or a variation therein)

PrintIntegers(const array[], size)
{
	for (new i=0; i<size; i++)
	{
		PrintToServer("array[%d] = %d", i, array[i])
	}
}

public Action:Command_TestSortInts(args)
{
	new array[10] = {6, 7, 3, 2, 8, 5, 0, 1, 4, 9}
	
	PrintToServer("Testing ascending sort:")
	SortIntegers(array, 10, Sort_Ascending)
	PrintIntegers(array, 10)
	
	PrintToServer("Testing descending sort:")
	SortIntegers(array, 10, Sort_Descending)
	PrintIntegers(array, 10)
	
	PrintToServer("Testing random sort:")
	SortIntegers(array, 10, Sort_Random)
	PrintIntegers(array, 10)
	
	return Plugin_Handled
}

/**************************
 * Float comparison tests *
 **************************/

PrintFloats(const Float:array[], size)
{
	for (new i=0; i<size; i++)
	{
		PrintToServer("array[%d] = %f", i, array[i])
	}
}

public Action:Command_TestSortFloats(args)
{
	new Float:array[10] = {6.3, 7.6, 3.2, 2.1, 8.5, 5.2, 0.4, 1.7, 4.8, 8.2}
	
	PrintToServer("Testing ascending sort:")
	SortFloats(array, 10, Sort_Ascending)
	PrintFloats(array, 10)
	
	PrintToServer("Testing descending sort:")
	SortFloats(array, 10, Sort_Descending)
	PrintFloats(array, 10)
	
	PrintToServer("Testing random sort:")
	SortFloats(array, 10, Sort_Random)
	PrintFloats(array, 10)
	
	return Plugin_Handled
}

public Custom1DSort(elem1, elem2, const array[], Handle:hndl)
{
	new Float:f1 = Float:elem1
	new Float:f2 = Float:elem2
	if (f1 > f2)
	{
		return -1;
	} else if (f1 < f2) {
		return 1;
	}
	
	return 0;
}

public Action:Command_TestSort1D(args)
{
	new Float:array[10] = {6.3, 7.6, 3.2, 2.1, 8.5, 5.2, 0.4, 1.7, 4.8, 8.2}
	
	SortCustom1D(_:array, 10, Custom1DSort)
	PrintFloats(array, 10)
	
	return Plugin_Handled
}

/***************************
 * String comparison tests *
 ***************************/

PrintStrings(const String:array[][], size)
{
	for (new i=0; i<size; i++)
	{
		PrintToServer("array[%d] = %s", i, array[i])
	}
}

public Action:Command_TestSortStrings(args)
{
	new String:strarray[][] = 
		{
			"faluco",
			"bailopan",
			"pm onoto",
			"damaged soul",
			"sniperbeamer",
			"sidluke",
			"johnny got his gun",
			"gabe newell",
			"pRED*'s awesome",
			"WHAT?!"
		}
		
	PrintToServer("Testing ascending sort:")
	SortStrings(strarray, 10, Sort_Ascending)
	PrintStrings(strarray, 10)
	
	PrintToServer("Testing descending sort:")
	SortStrings(strarray, 10, Sort_Descending)
	PrintStrings(strarray, 10)
	
	PrintToServer("Testing random sort:")
	SortStrings(strarray, 10, Sort_Random)
	PrintStrings(strarray, 10)
	
	return Plugin_Handled
}

public Custom2DSort(String:elem1[], String:elem2[], String:array[][], Handle:hndl)
{
	return strcmp(elem1, elem2)
}

public Action:Command_TestSort2D(args)
{
	new String:array[][] = 
		{
			"faluco",
			"bailopan",
			"pm onoto",
			"damaged soul",
			"sniperbeamer",
			"sidluke",
			"johnny got his gun",
			"gabe newell",
			"pred is a crab",
			"WHAT?!"
		}
	
	SortCustom2D(_:array, 10, Custom2DSort)
	PrintStrings(array, 10)
	
	return Plugin_Handled
}

/*******************
 * ADT ARRAY TESTS *
 *******************/
// Int and floats work the same as normal comparisions. Strings are direct
// comparisions with no hacky memory stuff like Pawn arrays.

PrintADTArrayIntegers(Handle:array)
{
	new size = GetArraySize(array);
	for (new i=0; i<size;i++)
	{
		PrintToServer("array[%d] = %d", i, GetArrayCell(array, i));	
	}
}

public Action:Command_TestSortADTInts(args)
{
	new Handle:array = CreateArray();
	PushArrayCell(array, 6);
	PushArrayCell(array, 7);
	PushArrayCell(array, 3);
	PushArrayCell(array, 2);
	PushArrayCell(array, 8);
	PushArrayCell(array, 5);
	PushArrayCell(array, 0);
	PushArrayCell(array, 1);
	PushArrayCell(array, 4);
	PushArrayCell(array, 9);
	
	PrintToServer("Testing ascending sort:")
	SortADTArray(array, Sort_Ascending, Sort_Integer)
	PrintADTArrayIntegers(array)
	
	PrintToServer("Testing descending sort:")
	SortADTArray(array, Sort_Descending, Sort_Integer)
	PrintADTArrayIntegers(array)
	
	PrintToServer("Testing random sort:")
	SortADTArray(array, Sort_Random, Sort_Integer)
	PrintADTArrayIntegers(array)
	
	return Plugin_Handled

}

PrintADTArrayFloats(Handle:array)
{
	new size = GetArraySize(array);
	for (new i=0; i<size;i++)
	{
		PrintToServer("array[%d] = %f", i, float:GetArrayCell(array, i));	
	}
}

public Action:Command_TestSortADTFloats(args)
{
	new Handle:array = CreateArray();
	PushArrayCell(array, 6.0);
	PushArrayCell(array, 7.0);
	PushArrayCell(array, 3.0);
	PushArrayCell(array, 2.0);
	PushArrayCell(array, 8.0);
	PushArrayCell(array, 5.0);
	PushArrayCell(array, 0.0);
	PushArrayCell(array, 1.0);
	PushArrayCell(array, 4.0);
	PushArrayCell(array, 9.0);
	
	PrintToServer("Testing ascending sort:")
	SortADTArray(array, Sort_Ascending, Sort_Float)
	PrintADTArrayFloats(array)
	
	PrintToServer("Testing descending sort:")
	SortADTArray(array, Sort_Descending, Sort_Float)
	PrintADTArrayFloats(array)
	
	PrintToServer("Testing random sort:")
	SortADTArray(array, Sort_Random, Sort_Float)
	PrintADTArrayFloats(array)
	
	return Plugin_Handled
}

PrintADTArrayStrings(Handle:array)
{
	new size = GetArraySize(array);
	decl String:buffer[64];
	for (new i=0; i<size;i++)
	{
		GetArrayString(array, i, buffer, sizeof(buffer));
		PrintToServer("array[%d] = %s", i, buffer);	
	}
}

public Action:Command_TestSortADTStrings(args)
{
	new Handle:array = CreateArray(ByteCountToCells(64));
	PushArrayString(array, "faluco");
	PushArrayString(array, "bailopan");
	PushArrayString(array, "pm onoto");
	PushArrayString(array, "damaged soul");
	PushArrayString(array, "sniperbeamer");
	PushArrayString(array, "sidluke");
	PushArrayString(array, "johnny got his gun");
	PushArrayString(array, "gabe newell");
	PushArrayString(array, "Hello pRED*");
	PushArrayString(array, "WHAT?!");
	
	PrintToServer("Testing ascending sort:")
	SortADTArray(array, Sort_Ascending, Sort_String)
	PrintADTArrayStrings(array)
	
	PrintToServer("Testing descending sort:")
	SortADTArray(array, Sort_Descending, Sort_String)
	PrintADTArrayStrings(array)
	
	PrintToServer("Testing random sort:")
	SortADTArray(array, Sort_Random, Sort_String)
	PrintADTArrayStrings(array)
	
	return Plugin_Handled
}

public ArrayADTCustomCallback(index1, index2, Handle:array, Handle:hndl)
{
	decl String:buffer1[64], String:buffer2[64];
	GetArrayString(array, index1, buffer1, sizeof(buffer1));
	GetArrayString(array, index2, buffer2, sizeof(buffer2));
	
	return strcmp(buffer1, buffer2);
}

public Action:Command_TestSortADTCustom(args)
{
	new Handle:array = CreateArray(ByteCountToCells(64));
	PushArrayString(array, "faluco");
	PushArrayString(array, "bailopan");
	PushArrayString(array, "pm onoto");
	PushArrayString(array, "damaged soul");
	PushArrayString(array, "sniperbeamer");
	PushArrayString(array, "sidluke");
	PushArrayString(array, "johnny got his gun");
	PushArrayString(array, "gabe newell");
	PushArrayString(array, "pRED*'s running out of ideas");
	PushArrayString(array, "WHAT?!");
	
	PrintToServer("Testing custom sort:")
	SortADTArrayCustom(array, ArrayADTCustomCallback)
	PrintADTArrayStrings(array);
}
