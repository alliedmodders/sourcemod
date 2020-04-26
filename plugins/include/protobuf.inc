/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod (C)2013 AlliedModders LLC.  All rights reserved.
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

#if defined _protobuf_included
 #endinput
#endif
#define _protobuf_included

#define PB_FIELD_NOT_REPEATED -1

methodmap Protobuf < Handle
{
	// Reads an int32, uint32, sint32, fixed32, sfixed32, or enum value from a protobuf message.
	//
	// @param field      Field name.
	// @param index      Index into repeated field.
	// @return           Integer value read.
	// @error            Non-existent field, or incorrect field type.
	public native int ReadInt(const char[] field, int index = PB_FIELD_NOT_REPEATED);

	// Reads an int64, uint64, sint64, fixed64, sfixed64 from a protobuf message.
	//
	// @param field      Field name.
	// @param value      Array to represent the large integer (0=High bits, 1=Low bits).
	// @param index      Index into repeated field.
	// @error            Non-existent field, or incorrect field type.
	public native void ReadInt64(const char[] field, int value[2], int index = PB_FIELD_NOT_REPEATED);

	// Reads a float or downcasted double from a protobuf message.
	//
	// @param field      Field name.
	// @param index      Index into repeated field.
	// @return           Float value read.
	// @error            Non-existent field, or incorrect field type.
	public native float ReadFloat(const char[] field, int index = PB_FIELD_NOT_REPEATED);

	// Reads a bool from a protobuf message.
	//
	// @param field      Field name.
	// @param index      Index into repeated field.
	// @return           Boolean value read.
	// @error            Non-existent field, or incorrect field type.
	public native bool ReadBool(const char[] field, int index = PB_FIELD_NOT_REPEATED);

	// Reads a string from a protobuf message.
	//
	// @param field      Field name.
	// @param buffer     Destination string buffer.
	// @param maxlength  Maximum length of output string buffer.
	// @param index      Index into repeated field.
	// @error            Non-existent field, or incorrect field type.
	public native void ReadString(const char[] field, char[] buffer, int maxlength, int index = PB_FIELD_NOT_REPEATED);

	// Reads an RGBA color value from a protobuf message.
	//
	// @param field      Field name.
	// @param buffer     Destination color buffer.
	// @param index      Index into repeated field.
	// @error            Non-existent field, or incorrect field type.
	public native void ReadColor(const char[] field, int buffer[4], int index = PB_FIELD_NOT_REPEATED);

	// Reads an XYZ angle value from a protobuf message.
	//
	// @param field      Field name.
	// @param buffer     Destination angle buffer.
	// @param index      Index into repeated field.
	// @error            Non-existent field, or incorrect field type.
	public native void ReadAngle(const char[] field, float buffer[3], int index = PB_FIELD_NOT_REPEATED);

	// Reads an XYZ vector value from a protobuf message.
	//
	// @param pb         protobuf handle.
	// @param field      Field name.
	// @param buffer     Destination vector buffer.
	// @param index      Index into repeated field.
	// @error            Non-existent field, or incorrect field type.
	public native void ReadVector(const char[] field, float buffer[3], int index = PB_FIELD_NOT_REPEATED);

	// Reads an XY vector value from a protobuf message.
	//
	// @param field      Field name.
	// @param buffer     Destination vector buffer.
	// @param index      Index into repeated field.
	// @error            Non-existent field, or incorrect field type.
	public native void ReadVector2D(const char[] field, float buffer[2], int index = PB_FIELD_NOT_REPEATED);

	// Gets the number of elements in a repeated field of a protobuf message.
	//
	// @param field      Field name.
	// @return           Number of elements in the field.
	// @error            Non-existent field, or non-repeated field.
	public native int GetRepeatedFieldCount(const char[] field);

	// Returns whether or not the named, non-repeated field has a value set.
	//
	// @param field      Field name.
	// @return           True if value has been set, else false.
	// @error            Non-existent field, or repeated field.
	public native bool HasField(const char[] field);

	// Sets an int32, uint32, sint32, fixed32, sfixed32, or enum value on a protobuf message.
	//
	// @param field      Field name.
	// @param value      Integer value to set.
	// @param index      Index into repeated field.
	// @error            Non-existent field, or incorrect field type.
	public native void SetInt(const char[] field, int value, int index = PB_FIELD_NOT_REPEATED);

	// Sets an int64, uint64, sint64, fixed64, sfixed64 on a protobuf message.
	//
	// @param field      Field name.
	// @param value      Large integer value to set (0=High bits, 1=Low bits).
	// @param index      Index into repeated field.
	// @error            Non-existent field, or incorrect field type.
	public native void SetInt64(const char[] field, int value[2], int index = PB_FIELD_NOT_REPEATED);

	// Sets a float or double on a protobuf message.
	//
	// @param field      Field name.
	// @param value      Float value to set.
	// @param index      Index into repeated field.
	// @error            Non-existent field, or incorrect field type.
	public native void SetFloat(const char[] field, float value, int index = PB_FIELD_NOT_REPEATED);

	// Sets a bool on a protobuf message.
	//
	// @param field      Field name.
	// @param value      Boolean value to set.
	// @param index      Index into repeated field.
	// @error            Non-existent field, or incorrect field type.
	public native void SetBool(const char[] field, bool value, int index = PB_FIELD_NOT_REPEATED);

	// Sets a string on a protobuf message.
	//
	// @param field      Field name.
	// @param value      String value to set.
	// @param index      Index into repeated field.
	// @error            Non-existent field, or incorrect field type.
	public native void SetString(const char[] field, const char[] value, int index = PB_FIELD_NOT_REPEATED);

	// Sets an RGBA color on a protobuf message.
	//
	// @param field      Field name.
	// @param color      Color value to set.
	// @param index      Index into repeated field.
	// @error            Non-existent field, or incorrect field type.
	public native void SetColor(const char[] field, const int color[4], int index = PB_FIELD_NOT_REPEATED);

	// Sets an XYZ angle on a protobuf message.
	//
	// @param field      Field name.
	// @param angle      Angle value to set.
	// @param index      Index into repeated field.
	// @error            Non-existent field, or incorrect field type.
	public native void SetAngle(const char[] field, const float angle[3], int index = PB_FIELD_NOT_REPEATED);

	// Sets an XYZ vector on a protobuf message.
	//
	// @param field      Field name.
	// @param vec        Vector value to set.
	// @param index      Index into repeated field.
	// @error            Non-existent field, or incorrect field type.
	public native void SetVector(const char[] field, const float vec[3], int index = PB_FIELD_NOT_REPEATED);

	// Sets an XY vector on a protobuf message.
	//
	// @param field      Field name.
	// @param vec        Vector value to set.
	// @param index      Index into repeated field.
	// @error            Non-existent field, or incorrect field type.
	public native void SetVector2D(const char[] field, const float vec[2], int index = PB_FIELD_NOT_REPEATED);

	// Add an int32, uint32, sint32, fixed32, sfixed32, or enum value to a protobuf message repeated field.
	//
	// @param field      Field name.
	// @param value      Integer value to add.
	// @error            Non-existent field, or incorrect field type.
	public native void AddInt(const char[] field, int value);

	// Add an int64, uint64, sint64, fixed64, sfixed64 to a protobuf message repeated field.
	//
	// @param field      Field name.
	// @param value      Large integer value to add (0=High bits, 1=Low bits).
	// @error            Non-existent field, or incorrect field type.
	public native void AddInt64(const char[] field, int value[2]);

	// Add a float or double to a protobuf message repeated field.
	//
	// @param field      Field name.
	// @param value      Float value to add.
	// @error            Non-existent field, or incorrect field type.
	public native void AddFloat(const char[] field, float value);

	// Add a bool to a protobuf message repeated field.
	//
	// @param field      Field name.
	// @param value      Boolean value to add.
	// @error            Non-existent field, or incorrect field type.
	public native void AddBool(const char[] field, bool value);

	// Add a string to a protobuf message repeated field.
	//
	// @param field      Field name.
	// @param value      String value to add.
	// @error            Non-existent field, or incorrect field type.
	public native void AddString(const char[] field, const char[] value);

	// Add an RGBA color to a protobuf message repeated field.
	//
	// @param field      Field name.
	// @param color      Color value to add.
	// @error            Non-existent field, or incorrect field type.
	public native void AddColor(const char[] field, const int color[4]);

	// Add an XYZ angle to a protobuf message repeated field.
	//
	// @param field      Field name.
	// @param angle      Angle value to add.
	// @error            Non-existent field, or incorrect field type.
	public native void AddAngle(const char[] field, const float angle[3]);

	// Add an XYZ vector to a protobuf message repeated field.
	//
	// @param field      Field name.
	// @param vec        Vector value to add.
	// @error            Non-existent field, or incorrect field type.
	public native void AddVector(const char[] field, const float vec[3]);

	// Add an XY vector to a protobuf message repeated field.
	//
	// @param field      Field name.
	// @param vec        Vector value to add.
	// @error            Non-existent field, or incorrect field type.
	public native void AddVector2D(const char[] field, const float vec[2]);

	// Removes a value by index from a protobuf message repeated field.
	//
	// @param field      Field name.
	// @param index      Index into repeated field.
	// @error            Non-existent field, or incorrect field type.
	public native void RemoveRepeatedFieldValue(const char[] field, int index);

	// Retrieve a handle to an embedded protobuf message in a protobuf message.
	//
	// @param field      Field name.
	// @return           Protobuf handle to embedded message.
	// @error            Non-existent field, or incorrect field type.
	public native Protobuf ReadMessage(const char[] field);

	// Retrieve a handle to an embedded protobuf message in a protobuf message
	// repeated field.
	//
	// @param field      Field name.
	// @param index      Index in the repeated field.
	// @return           Protobuf handle to embedded message.
	// @error            Non-existent field, or incorrect field type.
	public native Protobuf ReadRepeatedMessage(const char[] field, int index);

	// Adds an embedded protobuf message to a protobuf message repeated field.
	//
	// @param field      Field name.
	// @return           Protobuf handle to added, embedded message.
	// @error            Non-existent field, or incorrect field type.
	public native Protobuf AddMessage(const char[] field);
};

/**
 * Reads an int32, uint32, sint32, fixed32, sfixed32, or enum value from a protobuf message.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param index         Index into repeated field.
 * @return              Integer value read.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native int PbReadInt(Handle pb, const char[] field, int index = PB_FIELD_NOT_REPEATED);

/**
 * Reads a float or downcasted double from a protobuf message.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param index         Index into repeated field.
 * @return              Float value read.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native float PbReadFloat(Handle pb, const char[] field, int index = PB_FIELD_NOT_REPEATED);

/**
 * Reads a bool from a protobuf message.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param index         Index into repeated field.
 * @return              Boolean value read.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native bool PbReadBool(Handle pb, const char[] field, int index = PB_FIELD_NOT_REPEATED);

/**
 * Reads a string from a protobuf message.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param buffer        Destination string buffer.
 * @param maxlength     Maximum length of output string buffer.
 * @param index         Index into repeated field.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbReadString(Handle pb, const char[] field, char[] buffer, int maxlength, int index = PB_FIELD_NOT_REPEATED);

/**
 * Reads an RGBA color value from a protobuf message.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param buffer        Destination color buffer.
 * @param index         Index into repeated field.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbReadColor(Handle pb, const char[] field, int buffer[4], int index = PB_FIELD_NOT_REPEATED);

/**
 * Reads an XYZ angle value from a protobuf message.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param buffer        Destination angle buffer.
 * @param index         Index into repeated field.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbReadAngle(Handle pb, const char[] field, float buffer[3], int index = PB_FIELD_NOT_REPEATED);

/**
 * Reads an XYZ vector value from a protobuf message.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param buffer        Destination vector buffer.
 * @param index         Index into repeated field.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbReadVector(Handle pb, const char[] field, float buffer[3], int index = PB_FIELD_NOT_REPEATED);

/**
 * Reads an XY vector value from a protobuf message.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param buffer        Destination vector buffer.
 * @param index         Index into repeated field.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbReadVector2D(Handle pb, const char[] field, float buffer[2], int index = PB_FIELD_NOT_REPEATED);

/**
 * Gets the number of elements in a repeated field of a protobuf message.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @return              Number of elements in the field.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native int PbGetRepeatedFieldCount(Handle pb, const char[] field);

/**
 * Sets an int32, uint32, sint32, fixed32, sfixed32, or enum value on a protobuf message.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param value         Integer value to set.
 * @param index         Index into repeated field.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbSetInt(Handle pb, const char[] field, int value, int index = PB_FIELD_NOT_REPEATED);

/**
 * Sets a float or double on a protobuf message.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param value         Float value to set.
 * @param index         Index into repeated field.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbSetFloat(Handle pb, const char[] field, float value, int index = PB_FIELD_NOT_REPEATED);

/**
 * Sets a bool on a protobuf message.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param value         Boolean value to set.
 * @param index         Index into repeated field.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbSetBool(Handle pb, const char[] field, bool value, int index = PB_FIELD_NOT_REPEATED);

/**
 * Sets a string on a protobuf message.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param value         String value to set.
 * @param index         Index into repeated field.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbSetString(Handle pb, const char[] field, const char[] value, int index = PB_FIELD_NOT_REPEATED);

/**
 * Sets an RGBA color on a protobuf message.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param color         Color value to set.
 * @param index         Index into repeated field.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbSetColor(Handle pb, const char[] field, const int color[4], int index = PB_FIELD_NOT_REPEATED);

/**
 * Sets an XYZ angle on a protobuf message.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param angle         Angle value to set.
 * @param index         Index into repeated field.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbSetAngle(Handle pb, const char[] field, const float angle[3], int index = PB_FIELD_NOT_REPEATED);

/**
 * Sets an XYZ vector on a protobuf message.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param vec           Vector value to set.
 * @param index         Index into repeated field.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbSetVector(Handle pb, const char[] field, const float vec[3], int index = PB_FIELD_NOT_REPEATED);

/**
 * Sets an XY vector on a protobuf message.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param vec           Vector value to set.
 * @param index         Index into repeated field.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbSetVector2D(Handle pb, const char[] field, const float vec[2], int index = PB_FIELD_NOT_REPEATED);

/**
 * Add an int32, uint32, sint32, fixed32, sfixed32, or enum value to a protobuf message repeated field.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param value         Integer value to add.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbAddInt(Handle pb, const char[] field, int value);

/**
 * Add a float or double to a protobuf message repeated field.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param value         Float value to add.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbAddFloat(Handle pb, const char[] field, float value);

/**
 * Add a bool to a protobuf message repeated field.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param value         Boolean value to add.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbAddBool(Handle pb, const char[] field, bool value);

/**
 * Add a string to a protobuf message repeated field.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param value         String value to add.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbAddString(Handle pb, const char[] field, const char[] value);

/**
 * Add an RGBA color to a protobuf message repeated field.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param color         Color value to add.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbAddColor(Handle pb, const char[] field, const int color[4]);

/**
 * Add an XYZ angle to a protobuf message repeated field.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param angle         Angle value to add.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbAddAngle(Handle pb, const char[] field, const float angle[3]);

/**
 * Add an XYZ vector to a protobuf message repeated field.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param vec           Vector value to add.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbAddVector(Handle pb, const char[] field, const float vec[3]);

/**
 * Add an XY vector to a protobuf message repeated field.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param vec           Vector value to add.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbAddVector2D(Handle pb, const char[] field, const float vec[2]);

/**
 * Removes a value by index from a protobuf message repeated field.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param index         Index into repeated field.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native void PbRemoveRepeatedFieldValue(Handle pb, const char[] field, int index);

/**
 * Retrieve a handle to an embedded protobuf message in a protobuf message.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @return              protobuf handle to embedded message.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native Handle PbReadMessage(Handle pb, const char[] field);

/**
 * Retrieve a handle to an embedded protobuf message in a protobuf message repeated field.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @param index         Index in the repeated field.
 * @return              protobuf handle to embedded message.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native Handle PbReadRepeatedMessage(Handle pb, const char[] field, int index);

/**
 * Adds an embedded protobuf message to a protobuf message repeated field.
 *
 * @param pb            protobuf handle.
 * @param field         Field name.
 * @return              protobuf handle to added, embedded message.
 * @error               Invalid or incorrect Handle, non-existent field, or incorrect field type.
 */
native Handle PbAddMessage(Handle pb, const char[] field);
