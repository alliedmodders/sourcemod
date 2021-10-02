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

#if !defined __sourcepawn2__
/**
 * Converts an integer into a floating point value.
 *
 * @param value         Integer to convert.
 * @return              Floating point value.
 */
native float float(int value);
#endif

/**
 * Multiplies two floats together.
 *
 * Note: This native is internal implementation. For multiplication use the '*' operator.
 *
 * @param oper1         First value.
 * @param oper2         Second value.
 * @return              oper1*oper2.
 * @deprecated          This native is internal implementation. For multiplication use the '*' operator.
 */
#pragma deprecated This native is internal implementation. For multiplication use the '*' operator.
native float FloatMul(float oper1, float oper2);

/**
 * Divides the dividend by the divisor.
 *
 * Note: This native is internal implementation. For division use the '/' operator.
 *
 * @param dividend      First value.
 * @param divisor       Second value.
 * @return              dividend/divisor.
 * @deprecated          This native is internal implementation. For division use the '/' operator.
 */
#pragma deprecated This native is internal implementation. For division use the '/' operator.
native float FloatDiv(float dividend, float divisor);

/**
 * Adds two floats together.
 *
 * Note: This native is internal implementation. For addition use the '+' operator.
 *
 * @param oper1         First value.
 * @param oper2         Second value.
 * @return              oper1+oper2.
 * @deprecated          This native is internal implementation. For addition use the '+' operator.
 */
#pragma deprecated This native is internal implementation. For addition use the '+' operator.
native float FloatAdd(float oper1, float oper2);

/**
 * Subtracts oper2 from oper1.
 *
 * Note: This native is internal implementation. For subtraction use the '-' operator.
 *
 * @param oper1         First value.
 * @param oper2         Second value.
 * @return              oper1-oper2.
 * @deprecated          This native is internal implementation. For subtraction use the '-' operator.
 */
#pragma deprecated This native is internal implementation. For subtraction use the '-' operator.
native float FloatSub(float oper1, float oper2);

/**
 * Returns the decimal part of a float.
 *
 * @param value         Input value.
 * @return              Decimal part.
 */
native float FloatFraction(float value);

/**
 * Rounds a float to the closest integer to zero.
 *
 * @param value         Input value to be rounded.
 * @return              Rounded value.
 */
native int RoundToZero(float value);

/**
 * Rounds a float to the next highest integer value.
 *
 * @param value         Input value to be rounded.
 * @return              Rounded value.
 */
native int RoundToCeil(float value);

/**
 * Rounds a float to the next lowest integer value.
 *
 * @param value         Input value to be rounded.
 * @return              Rounded value.
 */
native int RoundToFloor(float value);

/**
 * Standard IEEE rounding.
 *
 * @param value         Input value to be rounded.
 * @return              Rounded value.
 */
native int RoundToNearest(float value);

/**
 * Compares two floats.
 *
 * @param fOne          First value.
 * @param fTwo          Second value.
 * @return              Returns 1 if the first argument is greater than the second argument.
 *                      Returns -1 if the first argument is smaller than the second argument.
 *                      Returns 0 if both arguments are equal.
 */
native int FloatCompare(float fOne, float fTwo);

/**
 * Returns the square root of the input value, equivalent to floatpower(value, 0.5).
 *
 * @param value         Input value.
 * @return              Square root of the value.
 */
native float SquareRoot(float value);

/**
 * Returns the value raised to the power of the exponent.
 *
 * @param value         Value to be raised.
 * @param exponent      Value to raise the base.
 * @return              value^exponent.
 */
native float Pow(float value, float exponent);

/**
 * Returns the value of raising the input by e.
 *
 * @param value         Input value.
 * @return              exp(value).
 */
native float Exponential(float value);

/**
 * Returns the logarithm of any base specified.
 *
 * @param value         Input value.
 * @param base          Logarithm base to use, default is 10.
 * @return              log(value)/log(base).
 */
native float Logarithm(float value, float base=10.0);

/**
 * Returns the sine of the argument.
 *
 * @param value         Input value in radians.
 * @return              sin(value).
 */
native float Sine(float value);

/**
 * Returns the cosine of the argument.
 *
 * @param value         Input value in radians.
 * @return              cos(value).
 */
native float Cosine(float value);

/**
 * Returns the tangent of the argument.
 *
 * @param value         Input value in radians.
 * @return              tan(value).
 */
native float Tangent(float value);

/**
 * Returns an absolute value.
 *
 * @param value         Input value.
 * @return              Absolute value of the input.
 */
native float FloatAbs(float value);

/**
 * Returns the arctangent of the input value.
 *
 * @param angle         Input value.
 * @return              atan(value) in radians.
 */
native float ArcTangent(float angle);

/**
 * Returns the arccosine of the input value.
 *
 * @param angle         Input value.
 * @return              acos(value) in radians.
 */
native float ArcCosine(float angle);

/**
 * Returns the arcsine of the input value.
 *
 * @param angle         Input value.
 * @return              asin(value) in radians.
 */
native float ArcSine(float angle);

/**
 * Returns the arctangent2 of the input values.
 *
 * @param x             Horizontal value.
 * @param y             Vertical value.
 * @return              atan2(value) in radians.
 */
native float ArcTangent2(float x, float y);

/**
 * Rounds a floating point number using the "round to nearest" algorithm.
 *
 * @param value         Floating point value to round.
 * @return              The value rounded to the nearest integer.
 */
stock int RoundFloat(float value)
{
	return RoundToNearest(value);
}

/**
 * User defined operators.
 */
#if !defined __sourcepawn2__
#pragma rational Float

// Internal aliases for backwards compatibility.
native float __FLOAT_MUL__(float a, float b) = FloatMul;
native float __FLOAT_DIV__(float a, float b) = FloatDiv;
native float __FLOAT_ADD__(float a, float b) = FloatAdd;
native float __FLOAT_SUB__(float a, float b) = FloatSub;

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
	return oper^view_as<float>(cellmin);                /* IEEE values are sign/magnitude */
}

// The stocks below are int->float converting versions of the above natives.

stock float operator*(float oper1, int oper2)
{
	return __FLOAT_MUL__(oper1, float(oper2));          /* "*" is commutative */
}

stock float operator/(float oper1, int oper2)
{
	return __FLOAT_DIV__(oper1, float(oper2));
}

stock float operator/(int oper1, float oper2)
{
	return __FLOAT_DIV__(float(oper1), oper2);
}

stock float operator+(float oper1, int oper2)
{
	return __FLOAT_ADD__(oper1, float(oper2));          /* "+" is commutative */
}

stock float operator-(float oper1, int oper2)
{
	return __FLOAT_SUB__(oper1, float(oper2));
}

stock float operator-(int oper1, float oper2)
{
	return __FLOAT_SUB__(float(oper1), oper2);
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
 */
forward operator%(float oper1, float oper2);
forward operator%(float oper1, int oper2);
forward operator%(int oper1, float oper2);
#endif // __sourcepawn2__

#define FLOAT_PI 3.1415926535897932384626433832795

/**
 * Converts degrees to radians.
 *
 * @param angle         Degrees.
 * @return              Radians.
 */
stock float DegToRad(float angle)
{
	return (angle*FLOAT_PI)/180;
}

/**
 * Converts radians to degrees.
 *
 * @param angle         Radians.
 * @return              Degrees.
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
 * @return              Random integer.
 */
native int GetURandomInt();

/**
 * Returns a uniform random float in the range [0, 1).
 *
 * Note: Uniform random number streams are seeded automatically per-plugin.
 *
 * @return              Uniform random floating-point number.
 */
native float GetURandomFloat();

/**
 * Seeds a plugin's uniform random number stream. This is done automatically,
 * so normally it is totally unnecessary to call this.
 *
 * @param seeds         Array of numbers to use as seeding data.
 * @param numSeeds      Number of seeds in the seeds array.
 */
native void SetURandomSeed(const int[] seeds, int numSeeds);

/**
 * Seeds a plugin's uniform random number stream. This is done automatically,
 * so normally it is totally unnecessary to call this.
 *
 * @param seed      Single seed value.
 */
stock void SetURandomSeedSimple(int seed)
{
	int seeds[1];
	seeds[0] = seed;
	SetURandomSeed(seeds, 1);
}
