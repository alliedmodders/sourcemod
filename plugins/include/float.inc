/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod (C)2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the SourceMod/SourcePawn SDK.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */
 
#if defined _float_included
 #endinput
#endif
#define _float_included

#include <float>
#include <handles>
#include <adt_array>
#include <logging>

#if !defined __sourcepawn2__
/**
 * Converts an integer into a floating point value.
 *
 * @param value			Integer to convert.
 * @return				Floating point value.
 */
native float float(int value);
#endif

/**
 * Multiplies two floats together.
 *
 * @param oper1			First value.
 * @param oper2			Second value.
 * @return				oper1*oper2.
 */
native float FloatMul(float oper1, float oper2);

/**
 * Divides the dividend by the divisor.
 *
 * @param dividend		First value.
 * @param divisor		Second value.
 * @return				dividend/divisor.
 */
native float FloatDiv(float dividend, float divisor);

/**
 * Adds two floats together.
 *
 * @param oper1			First value.
 * @param oper2			Second value.
 * @return				oper1+oper2.
 */
native float FloatAdd(float oper1, float oper2);

/**
 * Subtracts oper2 from oper1.
 *
 * @param oper1			First value.
 * @param oper2			Second value.
 * @return				oper1-oper2.
 */
native float FloatSub(float oper1, float oper2);

/**
 * Returns the decimal part of a float.
 *
 * @param value			Input value.
 * @return				Decimal part.
 */
native float FloatFraction(float value);

/**
 * Rounds a float to the closest integer to zero.
 *
 * @param value			Input value to be rounded.
 * @return				Rounded value.
 */
native int RoundToZero(float value);

/**
 * Rounds a float to the next highest integer value.
 *
 * @param value			Input value to be rounded.
 * @return				Rounded value.
 */
native int RoundToCeil(float value);

/**
 * Rounds a float to the next lowest integer value.
 *
 * @param value			Input value to be rounded.
 * @return				Rounded value.
 */
native int RoundToFloor(float value);

/**
 * Standard IEEE rounding.
 *
 * @param value			Input value to be rounded.
 * @return				Rounded value.
 */
native int RoundToNearest(float value);

/**
 * Compares two floats.
 *
 * @param fOne			First value.
 * @param fTwo			Second value.
 * @return				Returns 1 if the first argument is greater than the second argument.
 *                      Returns -1 if the first argument is smaller than the second argument.
 *                      Returns 0 if both arguments are equal.
 */
native int FloatCompare(float fOne, float fTwo);

/**
 * Returns the square root of the input value, equivalent to floatpower(value, 0.5).
 *
 * @param value			Input value.
 * @return				Square root of the value.
 */
native float SquareRoot(float value);

/**
 * Returns the value raised to the power of the exponent.
 *
 * @param value			Value to be raised.
 * @param exponent		Value to raise the base.
 * @return				value^exponent.
 */
native float Pow(float value, float exponent);

/**
 * Returns the value of raising the input by e.
 *
 * @param value			Input value.
 * @return				exp(value).
 */
native float Exponential(float value);

/**
 * Returns the logarithm of any base specified.
 *
 * @param value			Input value.
 * @param base			Logarithm base to use, default is 10.
 * @return				log(value)/log(base).
 */
native float Logarithm(float value, float base=10.0);

/**
 * Returns the sine of the argument.
 *
 * @param value			Input value in radians.
 * @return				sin(value).
 */
native float Sine(float value);

/**
 * Returns the cosine of the argument.
 *
 * @param value			Input value in radians.
 * @return				cos(value).
 */
native float Cosine(float value);

/**
 * Returns the tangent of the argument.
 *
 * @param value			Input value in radians.
 * @return				tan(value).
 */
native float Tangent(float value);

/**
 * Returns an absolute value.
 *
 * @param value			Input value.
 * @return				Absolute value of the input.
 */
native float FloatAbs(float value);

/**
 * Returns the arctangent of the input value.
 *
 * @param angle			Input value.
 * @return				atan(value) in radians.
 */
native float ArcTangent(float angle);

/**
 * Returns the arccosine of the input value.
 *
 * @param angle			Input value.
 * @return				acos(value) in radians.
 */
native float ArcCosine(float angle);

/**
 * Returns the arcsine of the input value.
 *
 * @param angle			Input value.
 * @return				asin(value) in radians.
 */
native float ArcSine(float angle);

/**
 * Returns the arctangent2 of the input values.
 *
 * @param x				Horizontal value.
 * @param y				Vertical value.
 * @return				atan2(value) in radians.
 */
native float ArcTangent2(float x, float y);

/**
 * Rounds a floating point number using the "round to nearest" algorithm.
 *
 * @param value			Floating point value to round.
 * @return				The value rounded to the nearest integer.
 */
stock int RoundFloat(float value)
{
	return RoundToNearest(value);
}

#define MAX_COMPLEXITY 2147483647

enum Operators
{
	Operator_None = 0,
	Operator_Add,
	Operator_Subtract,
	Operator_Multiply,
	Operator_Divide,
	Operator_Exponental,
};

/**
 * Called when eval runs into an error.
 *
 * @param error		Error that occurred.
 * @noreturn.
 */
typedef OnError = function void(const char[] error);

/**
 * Called when eval runs into an unknown character.
 *
 * @param variable		Variable that eval found.
 * @param value			Value to set if variable matches what you need.
 * @return				True if you got a value for variable, false otherwise.
 */
typedef OnVariable = function bool(const char variable[1], float &value);

stock void Operate(ArrayList sumArray, ArrayList operatorArray, int bracket, float value)
{
	float sum = sumArray.Get(bracket);
	switch (operatorArray.Get(bracket))
	{
		case Operator_Add:
			sum += value;
		case Operator_Subtract:
			sum -= value;
		case Operator_Multiply:
			sum *= value;
		case Operator_Divide:
		{
			if (FloatCompare(value, -0.0000001) == 1 && FloatCompare(value, 0.0000001) == -1)
				return;
			sum /= value;
		}
		case Operator_Exponental:
			sum = Pow(sum, value);
		default:
			sum = value;
	}
	sumArray.Set(bracket, sum);
	operatorArray.Set(bracket, Operator_None);
}

stock void OperateString(ArrayList sumArray, ArrayList operatorArray, int bracket, char[] value)
{
	if (StrEqual(value, ""))
		return;
	Operate(sumArray, operatorArray, bracket, StringToFloat(value));
	strcopy(value, strlen(value)+1, "");
}

/**
 * Parses a formula and return results.
 *
 * @param formula			Formula to parse.
 * @param errorCallback		Function to call incase of an error.
 * @param variableCallback	Function to call when an invalid character is found.
 * @return					Result of operation.
 */
stock float Eval(const char[] formula, OnError errorCallback, OnVariable variableCallback)
{
	int bracket;
	char buffer[2], strValue[1024];
	ArrayList sumArray = new ArrayList(.startsize=1);
	ArrayList operatorArray = new ArrayList(.startsize=1);
	for (int i = 0; i <= strlen(formula); i++)
	{
		buffer[0] = formula[i];
		switch (buffer[0])
		{
			case ' ': //Ignore whitespace.
				continue;
			case '\0':
				OperateString(sumArray, operatorArray, bracket, strValue);
			case '(':
			{
				if (bracket == MAX_COMPLEXITY)
				{
					int res;
					char error[1024];
					Format(error, sizeof(error), "Formula %s is too complex for this stock.", formula);
					Call_StartFunction(INVALID_HANDLE, errorCallback);
					Call_PushString(error);
					Call_Finish(res);
					return 0.0;
				}
				bracket++;
				if (bracket + 1 > sumArray.Length)
				{
					sumArray.Push(0.0);
					operatorArray.Push(Operator_None);
				}
				else
				{
					sumArray.Set(bracket, 0.0);
					operatorArray.Set(bracket, Operator_None);
				}
			}
			case ')':
			{
				if (bracket == 0)
				{
					int res;
					char error[1024];
					Format(error, sizeof(error), "Formula %s has too many ')'.", formula);
					Call_StartFunction(INVALID_HANDLE, errorCallback);
					Call_PushString(error);
					Call_Finish(res);
					return 0.0;
				}
				OperateString(sumArray, operatorArray, bracket, strValue);
				bracket--;
				Operate(sumArray, operatorArray, bracket, sumArray.Get(bracket+1));
			}
			case '+', '-', '*', '/', '^':
			{
				OperateString(sumArray, operatorArray, bracket, strValue);
				switch (buffer[0])
				{
					case '+':
						operatorArray.Set(bracket, Operator_Add);
					case '-':
						operatorArray.Set(bracket, Operator_Subtract);
					case '*':
						operatorArray.Set(bracket, Operator_Multiply);
					case '/':
						operatorArray.Set(bracket, Operator_Divide);
					case '^':
						operatorArray.Set(bracket, Operator_Exponental);
				}
			}
			case '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.':
				StrCat(strValue, sizeof(strValue), buffer);
			default:
			{
				float vValue = 0.0;
				bool result;
				Call_StartFunction(INVALID_HANDLE, variableCallback);
				Call_PushString(buffer[0]);
				Call_PushFloatRef(vValue);
				if (Call_Finish(result) != SP_ERROR_NONE && result)
					Operate(sumArray, operatorArray, bracket, vValue);
				else
				{
					int res;
					char error[1024];
					Format(error, sizeof(error), "Formula %s contains invalid character %c", formula, buffer[0]);
					Call_StartFunction(INVALID_HANDLE, errorCallback);
					Call_PushString(error);
					Call_Finish(res);
					return 0.0;
				}
			}
		}
	}
	if (bracket > 0)
	{
		int res;
		char error[1024];
		Format(error, sizeof(error), "Formula %s is invalid, missing some ')'", formula);
		Call_StartFunction(INVALID_HANDLE, errorCallback);
		Call_PushString(error);
		Call_Finish(res);
		return 0.0;
	}
	float ret = sumArray.Get(0);
	delete sumArray;
	delete operatorArray;
	return ret;
}

public bool NoVariable(const char variable[1], float &value)
{
	return false;
}

public void OnEvalError(const char[] error)
{
	LogError(error);
}

/**
 * A wrapper function to get around a compiler bug.
 * Bug in question: Function params causing compile error when passed a default value. Compile error: <variable> prototype doesn't match.
 */
stock float eval(const char[] formula)
{
	return Eval(formula, OnEvalError, NoVariable);
}

/**
 * User defined operators.
 *
 */
#if !defined __sourcepawn2__
#pragma rational Float

native bool __FLOAT_GT__(float a, float b);
native bool __FLOAT_GE__(float a, float b);
native bool __FLOAT_LT__(float a, float b);
native bool __FLOAT_LE__(float a, float b);
native bool __FLOAT_EQ__(float a, float b);
native bool __FLOAT_NE__(float a, float b);
native bool __FLOAT_NOT__(float a);

native float operator*(float oper1, float oper2) = FloatMul;
native float operator/(float oper1, float oper2) = FloatDiv;
native float operator+(float oper1, float oper2) = FloatAdd;
native float operator-(float oper1, float oper2) = FloatSub;
native bool operator!(float oper1) = __FLOAT_NOT__;
native bool operator>(float oper1, float oper2) = __FLOAT_GT__;
native bool operator>=(float oper1, float oper2) = __FLOAT_GE__;
native bool operator<(float oper1, float oper2) = __FLOAT_LT__;
native bool operator<=(float oper1, float oper2) = __FLOAT_LE__;
native bool operator!=(float oper1, float oper2) = __FLOAT_NE__;
native bool operator==(float oper1, float oper2) = __FLOAT_EQ__;

stock float operator++(float oper)
{
	return oper+1.0;
}

stock float operator--(float oper)
{
	return oper-1.0;
}

stock float operator-(float oper)
{
	return oper^float cellmin;				/* IEEE values are sign/magnitude */
}

stock float operator*(float oper1, int oper2)
{
	return FloatMul(oper1, float(oper2));			/* "*" is commutative */
}

stock float operator/(float oper1, int oper2)
{
	return FloatDiv(oper1, float(oper2));
}

stock float operator/(int oper1, float oper2)
{
	return FloatDiv(float(oper1), oper2);
}

stock float operator+(float oper1, int oper2)
{
	return FloatAdd(oper1, float(oper2));			/* "+" is commutative */
}

stock float operator-(float oper1, int oper2)
{
	return FloatSub(oper1, float(oper2));
}

stock float operator-(int oper1, float oper2)
{
	return FloatSub(float(oper1), oper2);
}

stock bool operator==(float oper1, int oper2)
{
	return __FLOAT_EQ__(oper1, float(oper2));
}

stock bool operator!=(float oper1, int oper2)
{
	return __FLOAT_NE__(oper1, float(oper2));
}

stock bool operator>(float oper1, int oper2)
{
	return __FLOAT_GT__(oper1, float(oper2));
}

stock bool operator>(int oper1, float oper2)
{
	return __FLOAT_GT__(float(oper1), oper2);
}

stock bool operator>=(float oper1, int oper2)
{
	return __FLOAT_GE__(oper1, float(oper2));
}

stock bool operator>=(int oper1, float oper2)
{
	return __FLOAT_GE__(float(oper1), oper2);
}

stock bool operator<(float oper1, int oper2)
{
	return __FLOAT_LT__(oper1, float(oper2));
}

stock bool operator<(int oper1, float oper2)
{
	return __FLOAT_LT__(float(oper1), oper2);
}

stock bool operator<=(float oper1, int oper2)
{
	return __FLOAT_LE__(oper1, float(oper2));
}

stock bool operator<=(int oper1, float oper2)
{
	return __FLOAT_LE__(float(oper1), oper2);
}

/**
 * Forbidden operators.
 *
 */
forward operator%(float oper1, float oper2);
forward operator%(float oper1, int oper2);
forward operator%(int oper1, float oper2);
#endif // __sourcepawn2__

#define FLOAT_PI 3.1415926535897932384626433832795

/**
 * Converts degrees to radians.
 *
 * @param angle		Degrees.
 * @return			Radians.
 */
stock float DegToRad(float angle)
{
	return (angle*FLOAT_PI)/180;
}

/**
 * Converts degrees to radians.
 *
 * @param angle		Radians.
 * @return			Degrees.
 */
stock float RadToDeg(float angle)
{
	return (angle*180)/FLOAT_PI;
}

/**
 * Returns a random integer in the range [0, 2^31-1].
 *
 * Note: Uniform random number streams are seeded automatically per-plugin.
 *
 * @return			Random integer.
 */
native int GetURandomInt();

/**
 * Returns a uniform random float in the range [0, 1).
 *
 * Note: Uniform random number streams are seeded automatically per-plugin.
 *
 * @return			Uniform random floating-point number.
 */
native float GetURandomFloat();

/**
 * Seeds a plugin's uniform random number stream. This is done automatically,
 * so normally it is totally unnecessary to call this.
 *
 * @param seeds		Array of numbers to use as seeding data.
 * @param numSeeds	Number of seeds in the seeds array.
 * @noreturn
 */
native void SetURandomSeed(const seeds[], numSeeds);

/**
 * Seeds a plugin's uniform random number stream. This is done automatically,
 * so normally it is totally unnecessary to call this.
 *
 * @param seed      Single seed value.
 * @noreturn
 */
stock void SetURandomSeedSimple(seed)
{
	new seeds[1];
	seeds[0] = seed;
	SetURandomSeed(seeds, 1);
}

