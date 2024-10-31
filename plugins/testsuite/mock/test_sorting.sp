#pragma semicolon 1
#pragma newdecls required
#include <testing>

public Plugin myinfo = 
{
	name = "Sorting Test",
	author = "AlliedModders LLC",
	description = "Tests sorting functions",
	version = "1.0.0.0",
	url = "https://www.sourcemod.net/"
};

public void OnPluginStart() {
    SetTestContext("Sorting Test");
    Test_SortIntegers();
    Test_SortFloats();
    Test_SortStrings();
    Test_Sort1D();
    Test_Sort2D();
    Test_SortSortADTArrayIntegers();
    Test_SortSortADTArrayFloats();
    Test_SortSortADTArrayStrings();
    Test_SortADTCustom();
}

void Test_SortIntegers() {
    int array[] = {6, 7, 3, 2, 8, 5, 0, 1, 4, 9};
    SortIntegers(array, sizeof(array), Sort_Ascending);
    AssertArrayEq("SortIntegers Sort_Ascending", array, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, sizeof(array));
    SortIntegers(array, sizeof(array), Sort_Descending);
    AssertArrayEq("SortIntegers Sort_Descending", array, {9, 8, 7, 6, 5, 4, 3, 2, 1, 0}, sizeof(array));
}

void Test_SortFloats() {
    float array[] = {6.3, 7.6, 3.2, 2.1, 8.5, 5.2, 0.4, 1.7, 4.8, 8.2};
    SortFloats(array, sizeof(array), Sort_Ascending);
    AssertArrayEq("SortFloats Sort_Ascending", array, {0.4, 1.7, 2.1, 3.2, 4.8, 5.2, 6.3, 7.6, 8.2, 8.5}, sizeof(array));
    SortFloats(array, sizeof(array), Sort_Descending);
    AssertArrayEq("SortFloats Sort_Descending", array, {8.5, 8.2, 7.6, 6.3, 5.2, 4.8, 3.2, 2.1, 1.7, 0.4}, sizeof(array));
}

void Test_SortStrings() {
    char strarray[][] = {
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
    };
    char expected_ascending[][] = {
        "WHAT?!",
        "bailopan",
        "damaged soul",
        "faluco",
        "gabe newell",
        "johnny got his gun",
        "pRED*'s awesome",
        "pm onoto",
        "sidluke",
        "sniperbeamer"
    };
    SortStrings(strarray, sizeof(strarray), Sort_Ascending);
    AssertStrArrayEq("SortStrings Sort_Ascending", strarray, expected_ascending, sizeof(strarray));
    char expected_descending[][] = {
        "sniperbeamer",
        "sidluke",
        "pm onoto",
        "pRED*'s awesome",
        "johnny got his gun",
        "gabe newell",
        "faluco",
        "damaged soul",
		"bailopan",
        "WHAT?!",
    };
    SortStrings(strarray, sizeof(strarray), Sort_Descending);
    AssertStrArrayEq("SortStrings Sort_Descending", strarray, expected_descending, sizeof(strarray));
}

int Custom1DSort(int elem1, int elem2, const int[] array, Handle hndl) {
    return elem1 - elem2;
}

int Custom1DSortFloat(int elem1, int elem2, const int[] array, Handle hndl) {
    float f1 = view_as<float>(elem1);
    float f2 = view_as<float>(elem2);
    if (f1 > f2)
    {
        return 1;
    } else if (f1 < f2) {
        return -1;
    }
    return 0;
}

void Test_Sort1D() {
    int intArray[10] = {6, 7, 3, 2, 8, 5, 0, 1, 4, 9};
    SortCustom1D(intArray, sizeof(intArray), Custom1DSort);
    AssertArrayEq("SortCustom1D Integers Ascending", intArray, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, sizeof(intArray));

    float floatArray[10] = {6.3, 7.6, 3.2, 2.1, 8.5, 5.2, 0.4, 1.7, 4.8, 8.2};
    SortCustom1D(view_as<any>(floatArray), sizeof(floatArray), Custom1DSortFloat);
    AssertArrayEq("SortCustom1D Floats Ascending", floatArray, {0.4, 1.7, 2.1, 3.2, 4.8, 5.2, 6.3, 7.6, 8.2, 8.5}, sizeof(floatArray));
}

#define SUB_ARRAY_SIZE 4
int Custom2DSortInteger(int[] elem1, int[] elem2, const int[][] array, Handle hndl)
{
    int sum1, sum2;
    for (int i = 0; i < SUB_ARRAY_SIZE; ++i) {
        sum1 += elem1[i];
        sum2 += elem2[i];
    }
    return sum1 - sum2;
}

int Custom2DSortString(char[] elem1, char[] elem2, const char[][] array, Handle hndl)
{
    return strcmp(elem1, elem2);
}

void Test_Sort2D() {
    int array[][SUB_ARRAY_SIZE] = {
        { 5, 9, 2, 3 },
        { 10, 1, 24, 5 },
        { 1, 2, 3, 0}
    };
    int expected[][SUB_ARRAY_SIZE] = {
        { 1, 2, 3, 0},
        { 5, 9, 2, 3 },
        { 10, 1, 24, 5 }
    };
    SortCustom2D(array, sizeof(array), Custom2DSortInteger);
    AssertArray2DEq("SortCustom2D int[][] Ascending", array, expected, sizeof(array), sizeof(array[]));

    char strarray[][] = {
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
    };
    char expected_ascending[][] = {
        "WHAT?!",
        "bailopan",
        "damaged soul",
        "faluco",
        "gabe newell",
        "johnny got his gun",
        "pRED*'s awesome",
        "pm onoto",
        "sidluke",
        "sniperbeamer"
    };
    SortCustom2D(view_as<any>(strarray), sizeof(strarray), Custom2DSortString);
    AssertStrArrayEq("SortCustom2D char[][] Ascending", strarray, expected_ascending, sizeof(strarray));
}

void Test_SortSortADTArrayIntegers() {
    ArrayList arraylist = new ArrayList();
    arraylist.Push(6);
    arraylist.Push(7);
    arraylist.Push(3);
    arraylist.Push(2);
    arraylist.Push(8);
    arraylist.Push(5);
    arraylist.Push(0);
    arraylist.Push(4);
    arraylist.Push(9);
    arraylist.Push(1);

    arraylist.Sort(Sort_Ascending, Sort_Integer);
    int[] array1 = new int[arraylist.Length];
    for (int i = 0; i < arraylist.Length; ++i) {
        array1[i] = arraylist.Get(i);
    }
    AssertArrayEq("SortADTArray Integers Ascending", array1, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, arraylist.Length);

    arraylist.Sort(Sort_Descending, Sort_Integer);
    int[] array2 = new int[arraylist.Length];
    for (int i = 0; i < arraylist.Length; ++i) {
        array2[i] = arraylist.Get(i);
    }
    AssertArrayEq("SortADTArray Integers Descending", array2, {9, 8, 7, 6, 5, 4, 3, 2, 1, 0}, arraylist.Length);
}

void Test_SortSortADTArrayFloats() {
    ArrayList arraylist = new ArrayList();
    arraylist.Push(6.3);
    arraylist.Push(7.2);
    arraylist.Push(3.9);
    arraylist.Push(2.0);
    arraylist.Push(8.1);
    arraylist.Push(5.1);
    arraylist.Push(0.6);
    arraylist.Push(4.2);
    arraylist.Push(9.5);
    arraylist.Push(1.7);

    arraylist.Sort(Sort_Ascending, Sort_Float);
    float[] array1 = new float[arraylist.Length];
    for (int i = 0; i < arraylist.Length; ++i) {
        array1[i] = arraylist.Get(i);
    }
    AssertArrayEq("SortADTArray Float Ascending", array1, {0.6, 1.7, 2.0, 3.9, 4.2, 5.1, 6.3, 7.2, 8.1, 9.5}, arraylist.Length);

    arraylist.Sort(Sort_Descending, Sort_Float);
    float[] array2 = new float[arraylist.Length];
    for (int i = 0; i < arraylist.Length; ++i) {
        array2[i] = arraylist.Get(i);
    }
    AssertArrayEq("SortADTArray Float Descending", array2, {9.5, 8.1, 7.2, 6.3, 5.1, 4.2, 3.9, 2.0, 1.7, 0.6}, arraylist.Length);
}

void Test_SortSortADTArrayStrings() {
    ArrayList arraylist = new ArrayList(ByteCountToCells(64));
    arraylist.PushString("faluco");
    arraylist.PushString("bailopan");
    arraylist.PushString("pm onoto");
    arraylist.PushString("damaged soul");
    arraylist.PushString("sniperbeamer");
    arraylist.PushString("sidluke");
    arraylist.PushString("johnny got his gun");
    arraylist.PushString("gabe newell");
    arraylist.PushString("Hello pRED*");
    arraylist.PushString("WHAT?!");

    char expected_ascending[][] = {
        "Hello pRED*",
        "WHAT?!",
        "bailopan",
        "damaged soul",
        "faluco",
        "gabe newell",
        "johnny got his gun",
        "pm onoto",
        "sidluke",
        "sniperbeamer"
    };
    arraylist.Sort(Sort_Ascending, Sort_String);
    char[][] array1 = new char[arraylist.Length][64];
    for (int i = 0; i < arraylist.Length; ++i) {
        arraylist.GetString(i, array1[i], 64);
    }
    AssertStrArrayEq("SortADTArray String Ascending", array1, expected_ascending, arraylist.Length);

    char expected_descending[][] = {
        "sniperbeamer",
        "sidluke",
        "pm onoto",
        "johnny got his gun",
        "gabe newell",
        "faluco",
        "damaged soul",
        "bailopan",
        "WHAT?!",
        "Hello pRED*"
    };
    arraylist.Sort(Sort_Descending, Sort_String);
    char[][] array2 = new char[arraylist.Length][64];
    for (int i = 0; i < arraylist.Length; ++i) {
        arraylist.GetString(i, array2[i], 64);
    }
    AssertStrArrayEq("SortADTArray String Descending", array2, expected_descending, arraylist.Length);
}

enum struct Testy {
    int val;
    float fal;
}

int ArrayADTCustomCallback(int index1, int index2, ArrayList array, Handle hndl) {
    Testy testy1, testy2;
    array.GetArray(index1, testy1);
    array.GetArray(index2, testy2);
    return testy1.val - testy2.val;
}

void Test_SortADTCustom() {
    ArrayList arraylist = new ArrayList(sizeof(Testy));
    Testy testy1 = {11, 2.9};
    Testy testy2 = {5995, 0.0};
    Testy testy3 = {20, 66.0001};
    Testy testy4 = {185, -205.2};
    arraylist.PushArray(testy1);
    arraylist.PushArray(testy2);
    arraylist.PushArray(testy3);
    arraylist.PushArray(testy4);

    arraylist.SortCustom(ArrayADTCustomCallback);
    int[] array = new int[arraylist.Length];
    Testy temptesty;
    for (int i = 0; i < arraylist.Length; ++i) {
        arraylist.GetArray(i, temptesty);
        array[i] = temptesty.val;
    }
    AssertArrayEq("SortADTCustom enum struct Ascending", array, {11, 20, 185, 5995}, arraylist.Length);
}
