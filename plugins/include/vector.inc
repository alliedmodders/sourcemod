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

#if defined _vector_included
 #endinput
#endif
#define _vector_included

/**
 * Calculates a vector's length.
 *
 * @param vec           Vector.
 * @param squared       If true, the result will be squared (for optimization).
 * @return              Vector length (magnitude).
 */
native float GetVectorLength(const float vec[3], bool squared=false);

/**
 * Calculates the distance between two vectors.
 *
 * @param vec1          First vector.
 * @param vec2          Second vector.
 * @param squared       If true, the result will be squared (for optimization).
 * @return              Vector distance.
 */
native float GetVectorDistance(const float vec1[3], const float vec2[3], bool squared=false);

/**
 * Calculates the dot product of two vectors.
 *
 * @param vec1          First vector.
 * @param vec2          Second vector.
 * @return              Dot product of the two vectors.
 */
native float GetVectorDotProduct(const float vec1[3], const float vec2[3]);

/**
 * Computes the cross product of two vectors.  Any input array can be the same
 * as the output array.
 *
 * @param vec1          First vector.
 * @param vec2          Second vector.
 * @param result        Resultant vector.
 */
native void GetVectorCrossProduct(const float vec1[3], const float vec2[3], float result[3]);

/**
 * Normalizes a vector.  The input array can be the same as the output array.
 *
 * @param vec           Vector.
 * @param result        Resultant vector.
 * @return              Vector length.
 */
native float NormalizeVector(const float vec[3], float result[3]);

/**
 * Returns vectors in the direction of an angle.
 *
 * @param angle         Angle.
 * @param fwd           Forward vector buffer or NULL_VECTOR.
 * @param right         Right vector buffer or NULL_VECTOR.
 * @param up            Up vector buffer or NULL_VECTOR.
 */
native void GetAngleVectors(const float angle[3], float fwd[3], float right[3], float up[3]);

/**
 * Returns angles from a vector.
 *
 * @param vec           Vector.
 * @param angle         Angle buffer.
 */
native void GetVectorAngles(const float vec[3], float angle[3]);

/**
 * Returns direction vectors from a vector.
 *
 * @param vec           Vector.
 * @param right         Right vector buffer or NULL_VECTOR.
 * @param up            Up vector buffer or NULL_VECTOR.
 */
native void GetVectorVectors(const float vec[3], float right[3], float up[3]);

/**
 * Adds two vectors.  It is safe to use either input buffer as an output
 * buffer.
 *
 * @param vec1          First vector.
 * @param vec2          Second vector.
 * @param result        Result buffer.
 */
stock void AddVectors(const float vec1[3], const float vec2[3], float result[3])
{
	result[0] = vec1[0] + vec2[0];
	result[1] = vec1[1] + vec2[1];
	result[2] = vec1[2] + vec2[2];
}

/**
 * Subtracts a vector from another vector.  It is safe to use either input
 * buffer as an output buffer.
 *
 * @param vec1          First vector.
 * @param vec2          Second vector to subtract from first.
 * @param result        Result buffer.
 */
stock void SubtractVectors(const float vec1[3], const float vec2[3], float result[3])
{
	result[0] = vec1[0] - vec2[0];
	result[1] = vec1[1] - vec2[1];
	result[2] = vec1[2] - vec2[2];
}

/**
 * Scales a vector.
 *
 * @param vec           Vector.
 * @param scale         Scale value.
 */
stock void ScaleVector(float vec[3], float scale)
{
	vec[0] *= scale;
	vec[1] *= scale;
	vec[2] *= scale;
}

/**
 * Negatives a vector.
 *
 * @param vec           Vector.
 */
stock void NegateVector(float vec[3])
{
	vec[0] = -vec[0];
	vec[1] = -vec[1];
	vec[2] = -vec[2];
}

/**
 * Builds a vector from two points by subtracting the points.
 *
 * @param pt1           First point (to be subtracted from the second).
 * @param pt2           Second point.
 * @param output        Output vector buffer.
 */
stock void MakeVectorFromPoints(const float pt1[3], const float pt2[3], float output[3])
{
	output[0] = pt2[0] - pt1[0];
	output[1] = pt2[1] - pt1[1];
	output[2] = pt2[2] - pt1[2];
}
