/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod (C)2004-2014 AlliedModders LLC.  All rights reserved.
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

#if defined _bitbuffer_included
 #endinput
#endif
#define _bitbuffer_included

methodmap BfWrite < Handle
{
	// Writes a single bit to a writable bitbuffer (bf_write).
	//
	// @param bit       Bit to write (true for 1, false for 0).
	public native void WriteBool(bool bit);
	 
	// Writes a byte to a writable bitbuffer (bf_write).
	//
	// @param byte      Byte to write (value will be written as 8bit).
	public native void WriteByte(int byte);

	// Writes a byte to a writable bitbuffer (bf_write).
	//
	// @param chr       Character to write.
	public native void WriteChar(int chr);

	// Writes a 16bit integer to a writable bitbuffer (bf_write).
	//
	// @param num       Integer to write (value will be written as 16bit).
	public native void WriteShort(int num);

	// Writes a 16bit unsigned integer to a writable bitbuffer (bf_write).
	//
	// @param num       Integer to write (value will be written as 16bit).
	public native void WriteWord(int num);

	// Writes a normal integer to a writable bitbuffer (bf_write).
	//
	// @param num       Integer to write (value will be written as 32bit).
	public native void WriteNum(int num);

	// Writes a floating point number to a writable bitbuffer (bf_write).
	//
	// @param num       Number to write.
	public native void WriteFloat(float num);

	// Writes a string to a writable bitbuffer (bf_write).
	//
	// @param string    Text string to write.
	public native void WriteString(const char[] string);

	// Writes an entity to a writable bitbuffer (bf_write).
	//
	// @param ent       Entity index to write.
	public native void WriteEntity(int ent);

	// Writes a bit angle to a writable bitbuffer (bf_write).
	//
	// @param angle     Angle to write.
	// @param numBits   Optional number of bits to use.
	public native void WriteAngle(float angle, int numBits=8);

	// Writes a coordinate to a writable bitbuffer (bf_write).
	//
	// @param coord     Coordinate to write.
	public native void WriteCoord(float coord);

	// Writes a 3D vector of coordinates to a writable bitbuffer (bf_write).
	//
	// @param coord     Coordinate array to write.
	public native void WriteVecCoord(float coord[3]);

	// Writes a 3D normal vector to a writable bitbuffer (bf_write).
	//
	// @param vec       Vector to write.
	public native void WriteVecNormal(float vec[3]);

	// Writes a 3D angle vector to a writable bitbuffer (bf_write).
	//
	// @param angles    Angle vector to write.
	public native void WriteAngles(float angles[3]);
};

methodmap BfRead < Handle
{
	// Reads a single bit from a readable bitbuffer (bf_read).
	//
	// @return          Bit value read.
	public native bool ReadBool();

	// Reads a byte from a readable bitbuffer (bf_read).
	//
	// @return          Byte value read (read as 8bit).
	public native int ReadByte();

	// Reads a character from a readable bitbuffer (bf_read).
	//
	// @return          Character value read.
	public native int ReadChar();

	// Reads a 16bit integer from a readable bitbuffer (bf_read).
	//
	// @param bf        bf_read handle to read from.
	// @return          Integer value read (read as 16bit).
	public native int ReadShort();

	// Reads a 16bit unsigned integer from a readable bitbuffer (bf_read).
	//
	// @param bf        bf_read handle to read from.
	// @return          Integer value read (read as 16bit).
	public native int ReadWord();

	// Reads a normal integer to a readable bitbuffer (bf_read).
	//
	// @return          Integer value read (read as 32bit).
	public native int ReadNum();

	// Reads a floating point number from a readable bitbuffer (bf_read).
	//
	// @return          Floating point value read.
	public native float ReadFloat();

	// Reads a string from a readable bitbuffer (bf_read).
	//
	// @param buffer    Destination string buffer.
	// @param maxlength Maximum length of output string buffer.
	// @param line      If true the buffer will be copied until it reaches a '\n' or a null terminator.
	// @return          Number of bytes written to the buffer.  If the bitbuffer stream overflowed, 
	//                  that is, had no terminator before the end of the stream, then a negative 
	//                  number will be returned equal to the number of characters written to the 
	//                  buffer minus 1.  The buffer will be null terminated regardless of the 
	//                  return value.
	public native int ReadString(char[] buffer, int maxlength, bool line=false);

	// Reads an entity from a readable bitbuffer (bf_read).
	//
	// @return          Entity index read.
	public native int ReadEntity();

	// Reads a bit angle from a readable bitbuffer (bf_read).
	//
	// @param numBits   Optional number of bits to use.
	// @return          Angle read.
	public native float ReadAngle(int numBits=8);

	// Reads a coordinate from a readable bitbuffer (bf_read).
	//
	// @return          Coordinate read.
	public native float ReadCoord();

	// Reads a 3D vector of coordinates from a readable bitbuffer (bf_read).
	//
	// @param coord     Destination coordinate array.
	public native void ReadVecCoord(float coord[3]);

	// Reads a 3D normal vector from a readable bitbuffer (bf_read).
	//
	// @param vec       Destination vector array.
	public native void ReadVecNormal(float vec[3]);

	// Reads a 3D angle vector from a readable bitbuffer (bf_read).
	//
	// @param angles    Destination angle vector.
	public native void ReadAngles(float angles[3]);

	// Returns the number of bytes left in a readable bitbuffer (bf_read).
	property int BytesLeft {
		public native get();
	}
};

/** 
 * Writes a single bit to a writable bitbuffer (bf_write).
 *
 * @param bf            bf_write handle to write to.
 * @param bit           Bit to write (true for 1, false for 0).
 * @error               Invalid or incorrect Handle.
 */
native void BfWriteBool(Handle bf, bool bit);
 
/**
 * Writes a byte to a writable bitbuffer (bf_write).
 *
 * @param bf            bf_write handle to write to.
 * @param byte          Byte to write (value will be written as 8bit).
 * @error               Invalid or incorrect Handle.
 */
native void BfWriteByte(Handle bf, int byte);

/**
 * Writes a byte to a writable bitbuffer (bf_write).
 *
 * @param bf            bf_write handle to write to.
 * @param chr           Character to write.
 * @error               Invalid or incorrect Handle.
 */
native void BfWriteChar(Handle bf, int chr);

/**
 * Writes a 16bit integer to a writable bitbuffer (bf_write).
 *
 * @param bf            bf_write handle to write to.
 * @param num           Integer to write (value will be written as 16bit).
 * @error               Invalid or incorrect Handle.
 */
native void BfWriteShort(Handle bf, int num);

/**
 * Writes a 16bit unsigned integer to a writable bitbuffer (bf_write).
 *
 * @param bf            bf_write handle to write to.
 * @param num           Integer to write (value will be written as 16bit).
 * @error               Invalid or incorrect Handle.
 */
native void BfWriteWord(Handle bf, int num);

/**
 * Writes a normal integer to a writable bitbuffer (bf_write).
 *
 * @param bf            bf_write handle to write to.
 * @param num           Integer to write (value will be written as 32bit).
 * @error               Invalid or incorrect Handle.
 */
native void BfWriteNum(Handle bf, int num);

/**
 * Writes a floating point number to a writable bitbuffer (bf_write).
 *
 * @param bf            bf_write handle to write to.
 * @param num           Number to write.
 * @error               Invalid or incorrect Handle.
 */
native void BfWriteFloat(Handle bf, float num);

/**
 * Writes a string to a writable bitbuffer (bf_write).
 *
 * @param bf            bf_write handle to write to.
 * @param string        Text string to write.
 * @error               Invalid or incorrect Handle.
 */
native void BfWriteString(Handle bf, const char[] string);

/**
 * Writes an entity to a writable bitbuffer (bf_write).
 * @note This is a wrapper around BfWriteShort().
 *
 * @param bf            bf_write handle to write to.
 * @param ent           Entity index to write.
 * @error               Invalid or incorrect Handle, or invalid entity.
 */
native void BfWriteEntity(Handle bf, int ent);

/**
 * Writes a bit angle to a writable bitbuffer (bf_write).
 *
 * @param bf            bf_write handle to write to.
 * @param angle         Angle to write.
 * @param numBits       Optional number of bits to use.
 * @error               Invalid or incorrect Handle.
 */
native void BfWriteAngle(Handle bf, float angle, int numBits=8);

/**
 * Writes a coordinate to a writable bitbuffer (bf_write).
 *
 * @param bf            bf_write handle to write to.
 * @param coord         Coordinate to write.
 * @error               Invalid or incorrect Handle.
 */
native void BfWriteCoord(Handle bf, float coord);

/**
 * Writes a 3D vector of coordinates to a writable bitbuffer (bf_write).
 *
 * @param bf            bf_write handle to write to.
 * @param coord         Coordinate array to write.
 * @error               Invalid or incorrect Handle.
 */
native void BfWriteVecCoord(Handle bf, float coord[3]);

/**
 * Writes a 3D normal vector to a writable bitbuffer (bf_write).
 *
 * @param bf            bf_write handle to write to.
 * @param vec           Vector to write.
 * @error               Invalid or incorrect Handle.
 */
native void BfWriteVecNormal(Handle bf, float vec[3]);

/**
 * Writes a 3D angle vector to a writable bitbuffer (bf_write).
 *
 * @param bf            bf_write handle to write to.
 * @param angles        Angle vector to write.
 * @error               Invalid or incorrect Handle.
 */
native void BfWriteAngles(Handle bf, float angles[3]);

/** 
 * Reads a single bit from a readable bitbuffer (bf_read).
 *
 * @param bf            bf_read handle to read from.
 * @return              Bit value read.
 * @error               Invalid or incorrect Handle.
 */
native bool BfReadBool(Handle bf);

/**
 * Reads a byte from a readable bitbuffer (bf_read).
 *
 * @param bf            bf_read handle to read from.
 * @return              Byte value read (read as 8bit).
 * @error               Invalid or incorrect Handle.
 */
native int BfReadByte(Handle bf);

/**
 * Reads a character from a readable bitbuffer (bf_read).
 *
 * @param bf            bf_read handle to read from.
 * @return              Character value read.
 * @error               Invalid or incorrect Handle.
 */
native int BfReadChar(Handle bf);

/**
 * Reads a 16bit integer from a readable bitbuffer (bf_read).
 *
 * @param bf            bf_read handle to read from.
 * @return              Integer value read (read as 16bit).
 * @error               Invalid or incorrect Handle.
 */
native int BfReadShort(Handle bf);

/**
 * Reads a 16bit unsigned integer from a readable bitbuffer (bf_read).
 *
 * @param bf            bf_read handle to read from.
 * @return              Integer value read (read as 16bit).
 * @error               Invalid or incorrect Handle.
 */
native int BfReadWord(Handle bf);

/**
 * Reads a normal integer to a readable bitbuffer (bf_read).
 *
 * @param bf            bf_read handle to read from.
 * @return              Integer value read (read as 32bit).
 * @error               Invalid or incorrect Handle.
 */
native int BfReadNum(Handle bf);

/**
 * Reads a floating point number from a readable bitbuffer (bf_read).
 *
 * @param bf            bf_read handle to read from.
 * @return              Floating point value read.
 * @error               Invalid or incorrect Handle.
 */
native float BfReadFloat(Handle bf);

/**
 * Reads a string from a readable bitbuffer (bf_read).
 *
 * @param bf            bf_read handle to read from.
 * @param buffer        Destination string buffer.
 * @param maxlength     Maximum length of output string buffer.
 * @param line          If true the buffer will be copied until it reaches a '\n' or a null terminator.
 * @return              Number of bytes written to the buffer.  If the bitbuffer stream overflowed, 
 *                      that is, had no terminator before the end of the stream, then a negative 
 *                      number will be returned equal to the number of characters written to the 
 *                      buffer minus 1.  The buffer will be null terminated regardless of the 
 *                      return value.
 * @error               Invalid or incorrect Handle.
 */
native int BfReadString(Handle bf, char[] buffer, int maxlength, bool line=false);

/**
 * Reads an entity from a readable bitbuffer (bf_read).
 * @note This is a wrapper around BfReadShort().
 *
 * @param bf            bf_read handle to read from.
 * @return              Entity index read.
 * @error               Invalid or incorrect Handle.
 */
native int BfReadEntity(Handle bf);

/**
 * Reads a bit angle from a readable bitbuffer (bf_read).
 *
 * @param bf            bf_read handle to read from.
 * @param numBits       Optional number of bits to use.
 * @return              Angle read.
 * @error               Invalid or incorrect Handle.
 */
native float BfReadAngle(Handle bf, int numBits=8);

/**
 * Reads a coordinate from a readable bitbuffer (bf_read).
 *
 * @param bf            bf_read handle to read from.
 * @return              Coordinate read.
 * @error               Invalid or incorrect Handle.
 */
native float BfReadCoord(Handle bf);

/**
 * Reads a 3D vector of coordinates from a readable bitbuffer (bf_read).
 *
 * @param bf            bf_read handle to read from.
 * @param coord         Destination coordinate array.
 * @error               Invalid or incorrect Handle.
 */
native void BfReadVecCoord(Handle bf, float coord[3]);

/**
 * Reads a 3D normal vector from a readable bitbuffer (bf_read).
 *
 * @param bf            bf_read handle to read from.
 * @param vec           Destination vector array.
 * @error               Invalid or incorrect Handle.
 */
native void BfReadVecNormal(Handle bf, float vec[3]);

/**
 * Reads a 3D angle vector from a readable bitbuffer (bf_read).
 *
 * @param bf            bf_read handle to read from.
 * @param angles        Destination angle vector.
 * @error               Invalid or incorrect Handle.
 */
native void BfReadAngles(Handle bf, float angles[3]);

/**
 * Returns the number of bytes left in a readable bitbuffer (bf_read).
 *
 * @param bf            bf_read handle to read from.
 * @return              Number of bytes left unread.
 * @error               Invalid or incorrect Handle.
 */
native int BfGetNumBytesLeft(Handle bf);
