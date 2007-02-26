#include <sourcemod>

public Plugin:myinfo = 
{
	name = "Sorting Test",
	author = "AlliedModders LLC",
	description = "Tests sorting functions",
	version = "1.0.0.0",
	url = "http://www.sourcemod.net/"
};

public OnPluginStart(Handle:myself)
{
	RegServerCmd("test_sort_ints", Command_TestSortInts)
	RegServerCmd("test_sort_floats", Command_TestSortFloats)
	RegServerCmd("test_sort_strings", Command_TestSortStrings)
	RegServerCmd("test_sort_1d", Command_TestSort1D)
	RegServerCmd("test_sort_2d", Command_TestSort2D)
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
			"hello",
			"WHAT?!"
		}
		
	PrintToServer("Testing ascending sort:")
	SortStrings(strarray, 10, Sort_Ascending)
	PrintStrings(strarray, 10)
	
	PrintToServer("Testing descending sort:")
	SortStrings(strarray, 10, Sort_Descending)
	PrintStrings(strarray, 10)
	
	return Plugin_Handled
}

public Custom2DSort(String:elem1[], String:elem2[], String:array[][], Handle:hndl)
{
	return StrCompare(elem1, elem2)
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
			"hello",
			"WHAT?!"
		}
	
	SortCustom2D(_:array, 10, Custom2DSort)
	PrintStrings(array, 10)
	
	return Plugin_Handled
}
