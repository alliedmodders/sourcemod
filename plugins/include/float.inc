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

/**
 * Converts an integer into a floating point value.
 *
 * @param value			Integer to convert.
 * @return				Floating point value.
 */
native Float:float(value);

/**
 * Multiplies two floats together.
 *
 * @param oper1			First value.
 * @param oper2			Second value.
 * @return				oper1*oper2.
 */
native Float:FloatMul(Float:oper1, Float:oper2);

/**
 * Divides the dividend by the divisor.
 *
 * @param dividend		First value.
 * @param divisor		Second value.
 * @return				dividend/divisor.
 */
native Float:FloatDiv(Float:dividend, Float:divisor);

/**
 * Adds two floats together.
 *
 * @param oper1			First value.
 * @param oper2			Second value.
 * @return				oper1+oper2.
 */
native Float:FloatAdd(Float:oper1, Float:oper2);

/**
 * Subtracts oper2 from oper1.
 *
 * @param oper1			First value.
 * @param oper2			Second value.
 * @return				oper1-oper2.
 */
native Float:FloatSub(Float:oper1, Float:oper2);

/**
 * Returns the decimal part of a float.
 *
 * @param value			Input value.
 * @return				Decimal part.
 */
native Float:FloatFraction(Float:value);

/**
 * Rounds a float to the closest integer to zero.
 *
 * @param value			Input value to be rounded.
 * @return				Rounded value.
 */
native RoundToZero(Float:value);

/**
 * Rounds a float to the next highest integer value.
 *
 * @param value			Input value to be rounded.
 * @return				Rounded value.
 */
native RoundToCeil(Float:value);

/**
 * Rounds a float to the next lowest integer value.
 *
 * @param value			Input value to be rounded.
 * @return				Rounded value.
 */
native RoundToFloor(Float:value);

/**
 * Standard IEEE rounding.
 *
 * @param value			Input value to be rounded.
 * @return				Rounded value.
 */
native RoundToNearest(Float:value);

/**
 * Compares two floats.
 *
 * @param fOne			First value.
 * @param fTwo			Second value.
 * @return				Returns 1 if the first argument is greater than the second argument.
 *                      Returns -1 if the first argument is smaller than the second argument.
 *                      Returns 0 if both arguments are equal.
 */
native FloatCompare(Float:fOne, Float:fTwo);

/**
 * Returns the square root of the input value, equivalent to floatpower(value, 0.5).
 *
 * @param value			Input value.
 * @return				Square root of the value.
 */
native Float:SquareRoot(Float:value);

/**
 * Returns the value raised to the power of the exponent.
 *
 * @param value			Value to be raised.
 * @param exponent		Value to raise the base.
 * @return				value^exponent.
 */
native Float:Pow(Float:value, Float:exponent);

/**
 * Returns the value of raising the input by e.
 *
 * @param value			Input value.
 * @return				exp(value).
 */
native Float:Exponential(Float:value);

/**
 * Returns the logarithm of any base specified.
 *
 * @param value			Input value.
 * @param base			Logarithm base to use, default is 10.
 * @return				log(value)/log(base).
 */
native Float:Logarithm(Float:value, Float:base=10.0);

/**
 * Returns the sine of the argument.
 *
 * @param value			Input value in radians.
 * @return				sin(value).
 */
native Float:Sine(Float:value);

/**
 * Returns the cosine of the argument.
 *
 * @param value			Input value in radians.
 * @return				cos(value).
 */
native Float:Cosine(Float:value);

/**
 * Returns the tangent of the argument.
 *
 * @param value			Input value in radians.
 * @return				tan(value).
 */
native Float:Tangent(Float:value);

/**
 * Returns an absolute value.
 *
 * @param value			Input value.
 * @return				Absolute value of the input.
 */
native Float:FloatAbs(Float:value);

/**
 * Returns the arctangent of the input value.
 *
 * @param angle			Input value.
 * @return				atan(value) in radians.
 */
native Float:ArcTangent(Float:angle);

/**
 * Returns the arccosine of the input value.
 *
 * @param angle			Input value.
 * @return				acos(value) in radians.
 */
native Float:ArcCosine(Float:angle);

/**
 * Returns the arcsine of the input value.
 *
 * @param angle			Input value.
 * @return				asin(value) in radians.
 */
native Float:ArcSine(Float:angle);

/**
 * Returns the arctangent2 of the input values.
 *
 * @param x				Horizontal value.
 * @param y				Vertical value.
 * @return				atan2(value) in radians.
 */
native Float:ArcTangent2(Float:x, Float:y);

/**
 * Rounds a floating point number using the "round to nearest" algorithm.
 *
 * @param value			Floating point value to round.
 * @return				The value rounded to the nearest integer.
 */
stock RoundFloat(Float:value)
{
	return RoundToNearest(value);
}

/**
 * User defined operators.
 *
 */
#pragma rational Float

native Float:operator*(Float:oper1, Float:oper2) = FloatMul;
native Float:operator/(Float:oper1, Float:oper2) = FloatDiv;
native Float:operator+(Float:oper1, Float:oper2) = FloatAdd;
native Float:operator-(Float:oper1, Float:oper2) = FloatSub;

stock Float:operator++(Float:oper)
{
	return oper+1.0;
}

stock Float:operator--(Float:oper)
{
	return oper-1.0;
}

stock Float:operator-(Float:oper)
{
	return oper^Float:((-1)^((-1)/2));				/* IEEE values are sign/magnitude */
}

stock Float:operator*(Float:oper1, oper2)
{
	return FloatMul(oper1, float(oper2));			/* "*" is commutative */
}

stock Float:operator/(Float:oper1, oper2)
{
	return FloatDiv(oper1, float(oper2));
}

stock Float:operator/(oper1, Float:oper2)
{
	return FloatDiv(float(oper1), oper2);
}

stock Float:operator+(Float:oper1, oper2)
{
	return FloatAdd(oper1, float(oper2));			/* "+" is commutative */
}

stock Float:operator-(Float:oper1, oper2)
{
	return FloatSub(oper1, float(oper2));
}

stock Float:operator-(oper1, Float:oper2)
{
	return FloatSub(float(oper1), oper2);
}

stock bool:operator==(Float:oper1, Float:oper2)
{
	return FloatCompare(oper1, oper2) == 0;
}

stock bool:operator==(Float:oper1, oper2)
{
	return FloatCompare(oper1, float(oper2)) == 0; 	/* "==" is commutative */
}

stock bool:operator!=(Float:oper1, Float:oper2)
{
	return FloatCompare(oper1, oper2) != 0;
}

stock bool:operator!=(Float:oper1, oper2)
{
	return FloatCompare(oper1, float(oper2)) != 0; 	/* "==" is commutative */
}

stock bool:operator>(Float:oper1, Float:oper2)
{
	return FloatCompare(oper1, oper2) > 0;
}

stock bool:operator>(Float:oper1, oper2)
{
	return FloatCompare(oper1, float(oper2)) > 0;
}

stock bool:operator>(oper1, Float:oper2)
{
	return FloatCompare(float(oper1), oper2) > 0;
}

stock bool:operator>=(Float:oper1, Float:oper2)
{
	return FloatCompare(oper1, oper2) >= 0;
}

stock bool:operator>=(Float:oper1, oper2)
{
	return FloatCompare(oper1, float(oper2)) >= 0;
}

stock bool:operator>=(oper1, Float:oper2)
{
	return FloatCompare(float(oper1), oper2) >= 0;
}

stock bool:operator<(Float:oper1, Float:oper2)
{
	return FloatCompare(oper1, oper2) < 0;
}

stock bool:operator<(Float:oper1, oper2)
{
	return FloatCompare(oper1, float(oper2)) < 0;
}

stock bool:operator<(oper1, Float:oper2)
{
	return FloatCompare(float(oper1), oper2) < 0;
}

stock bool:operator<=(Float:oper1, Float:oper2)
{
	return FloatCompare(oper1, oper2) <= 0;
}

stock bool:operator<=(Float:oper1, oper2)
{
	return FloatCompare(oper1, float(oper2)) <= 0;
}

stock bool:operator<=(oper1, Float:oper2)
{
	return FloatCompare(float(oper1), oper2) <= 0;
}

stock bool:operator!(Float:oper)
{
	return (_:oper & ((-1)/2)) == 0;		/* -1 = all bits to 1; /2 = remove most significant bit (sign)
											works on both 32bit and 64bit systems; no constant required */
}

/**
 * Forbidden operators.
 *
 */
forward operator%(Float:oper1, Float:oper2);
forward operator%(Float:oper1, oper2);
forward operator%(oper1, Float:oper2);

#define FLOAT_PI 3.1415926535897932384626433832795

/**
 * Converts degrees to radians.
 *
 * @param angle		Degrees.
 * @return			Radians.
 */
stock Float:DegToRad(Float:angle)
{
	return (angle*FLOAT_PI)/180;
}

/**
 * Converts degrees to radians.
 *
 * @param angle		Radians.
 * @return			Degrees.
 */
stock Float:RadToDeg(Float:angle)
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
native GetURandomInt();

/**
 * Returns a uniform random float in the range [0, 1).
 *
 * Note: Uniform random number streams are seeded automatically per-plugin.
 *
 * @return			Uniform random floating-point number.
 */
native Float:GetURandomFloat();

/**
 * Seeds a plugin's uniform random number stream. This is done automatically,
 * so normally it is totally unnecessary to call this.
 *
 * @param seeds		Array of numbers to use as seeding data.
 * @param numSeeds	Number of seeds in the seeds array.
 * @noreturn
 */
native SetURandomSeed(const seeds[], numSeeds);

/**
 * Seeds a plugin's uniform random number stream. This is done automatically,
 * so normally it is totally unnecessary to call this.
 *
 * @param seed      Single seed value.
 * @noreturn
 */
stock SetURandomSeedSimple(seed)
{
	new seeds[1];
	seeds[0] = seed;
	SetURandomSeed(seeds, 1);
}

