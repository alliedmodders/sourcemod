/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be used 
 * or modified under the Terms and Conditions of its License Agreement, which is found 
 * in LICENSE.txt.  The Terms and Conditions for making SourceMod extensions/plugins 
 * may change at any time.  To view the latest information, see:
 *   http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */
 
#if defined _float_included
 #endinput
#endif
#define _float_included

/**
 * Different methods of rounding.
 */
enum floatround_method
{
	floatround_round = 0,		/**< Standard IEEE rounding */
	floatround_floor,			/**< Next lowest integer value. */
	floatround_ceil,			/**< Next highest integer value. */
	floatround_tozero			/** Closest integer to zero. */
};

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
 * Rounds a float into an integer number.
 *
 * @param value			Input value to be rounded.
 * @param method		Rounding method to use.
 * @return				Rounded value.
 */
native FloatRound(Float:value, floatround_method:method=floatround_round);

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
 * User defined operators.
 *
 */
#pragma rational Float

native Float:operator*(Float:oper1, Float:oper2) = FloatMul;
native Float:operator/(Float:oper1, Float:oper2) = FloatDiv;
native Float:operator+(Float:oper1, Float:oper2) = FloatAdd;
native Float:operator-(Float:oper1, Float:oper2) = FloatSub;

stock Float:operator++(Float:oper)
	return oper+1.0;

stock Float:operator--(Float:oper)
	return oper-1.0;

stock Float:operator-(Float:oper)
	return oper^Float:((-1)^((-1)/2));				/* IEEE values are sign/magnitude */

stock Float:operator*(Float:oper1, oper2)
	return FloatMul(oper1, float(oper2));			/* "*" is commutative */

stock Float:operator/(Float:oper1, oper2)
	return FloatDiv(oper1, float(oper2));

stock Float:operator/(oper1, Float:oper2)
	return FloatDiv(float(oper1), oper2);

stock Float:operator+(Float:oper1, oper2)
	return FloatAdd(oper1, float(oper2));			/* "+" is commutative */

stock Float:operator-(Float:oper1, oper2)
	return FloatSub(oper1, float(oper2));

stock Float:operator-(oper1, Float:oper2)
	return FloatSub(float(oper1), oper2);

stock bool:operator==(Float:oper1, Float:oper2)
	return FloatCompare(oper1, oper2) == 0;

stock bool:operator==(Float:oper1, oper2)
	return FloatCompare(oper1, float(oper2)) == 0; 	/* "==" is commutative */

stock bool:operator!=(Float:oper1, Float:oper2)
	return FloatCompare(oper1, oper2) != 0;

stock bool:operator!=(Float:oper1, oper2)
	return FloatCompare(oper1, float(oper2)) != 0; 	/* "==" is commutative */

stock bool:operator>(Float:oper1, Float:oper2)
	return FloatCompare(oper1, oper2) > 0;

stock bool:operator>(Float:oper1, oper2)
	return FloatCompare(oper1, float(oper2)) > 0;

stock bool:operator>(oper1, Float:oper2)
	return FloatCompare(float(oper1), oper2) > 0;

stock bool:operator>=(Float:oper1, Float:oper2)
	return FloatCompare(oper1, oper2) >= 0;

stock bool:operator>=(Float:oper1, oper2)
	return FloatCompare(oper1, float(oper2)) >= 0;

stock bool:operator>=(oper1, Float:oper2)
	return FloatCompare(float(oper1), oper2) >= 0;

stock bool:operator<(Float:oper1, Float:oper2)
	return FloatCompare(oper1, oper2) < 0;

stock bool:operator<(Float:oper1, oper2)
	return FloatCompare(oper1, float(oper2)) < 0;

stock bool:operator<(oper1, Float:oper2)
	return FloatCompare(float(oper1), oper2) < 0;

stock bool:operator<=(Float:oper1, Float:oper2)
	return FloatCompare(oper1, oper2) <= 0;

stock bool:operator<=(Float:oper1, oper2)
	return FloatCompare(oper1, float(oper2)) <= 0;

stock bool:operator<=(oper1, Float:oper2)
	return FloatCompare(float(oper1), oper2) <= 0;

stock bool:operator!(Float:oper)
	return (_:oper & ((-1)/2)) == 0;		/* -1 = all bits to 1; /2 = remove most significant bit (sign)
											works on both 32bit and 64bit systems; no constant required */

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
