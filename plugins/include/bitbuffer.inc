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

#if defined _bitbuffer_included
 #endinput
#endif
#define _bitbuffer_included

/** 
 * Writes a single bit to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param bit			Bit to write (true for 1, false for 0).
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteBool(Handle:bf, bool:bit);
 
/**
 * Writes a byte to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param byte			Byte to write (value will be written as 8bit).
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteByte(Handle:bf, byte);

/**
 * Writes a byte to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param chr			Character to write.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteChar(Handle:bf, chr);

/**
 * Writes a 16bit integer to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param num			Integer to write (value will be written as 16bit).
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteShort(Handle:bf, num);

/**
 * Writes a 16bit unsigned integer to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param num			Integer to write (value will be written as 16bit).
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteWord(Handle:bf, num);

/**
 * Writes a normal integer to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param num			Integer to write (value will be written as 32bit).
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteNum(Handle:bf, num);

/**
 * Writes a floating point number to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param num			Number to write.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteFloat(Handle:bf, Float:num);

/**
 * Writes a string to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param string		Text string to write.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteString(Handle:bf, const String:string[]);

/**
 * Writes an entity to a writable bitbuffer (bf_write).
 * @note This is a wrapper around BfWriteShort().
 *
 * @param bf			bf_write handle to write to.
 * @param ent			Entity index to write.
 * @noreturn
 * @error				Invalid or incorrect Handle, or invalid entity.
 */
native BfWriteEntity(Handle:bf, ent);

/**
 * Writes a bit angle to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param angle			Angle to write.
 * @param numBits		Optional number of bits to use.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteAngle(Handle:bf, Float:angle, numBits=8);

/**
 * Writes a coordinate to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param coord			Coordinate to write.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteCoord(Handle:bf, Float:coord);

/**
 * Writes a 3D vector of coordinates to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param coord			Coordinate array to write.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteVecCoord(Handle:bf, Float:coord[3]);

/**
 * Writes a 3D normal vector to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param vec			Vector to write.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteVecNormal(Handle:bf, Float:vec[3]);

/**
 * Writes a 3D angle vector to a writable bitbuffer (bf_write).
 *
 * @param bf			bf_write handle to write to.
 * @param angles		Angle vector to write.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteAngles(Handle:bf, Float:angles[3]);

/** 
 * Reads a single bit from a readable bitbuffer (bf_read).
 *
 * @param bf			bf_read handle to read from.
 * @return				Bit value read.
 * @error				Invalid or incorrect Handle.
 */
native bool:BfReadBool(Handle:bf);

/**
 * Reads a byte from a readable bitbuffer (bf_read).
 *
 * @param bf			bf_read handle to read from.
 * @return				Byte value read (read as 8bit).
 * @error				Invalid or incorrect Handle.
 */
native BfReadByte(Handle:bf);

/**
 * Reads a character from a readable bitbuffer (bf_read).
 *
 * @param bf			bf_read handle to read from.
 * @return				Character value read.
 * @error				Invalid or incorrect Handle.
 */
native BfReadChar(Handle:bf);

/**
 * Reads a 16bit integer from a readable bitbuffer (bf_read).
 *
 * @param bf			bf_read handle to read from.
 * @return				Integer value read (read as 16bit).
 * @error				Invalid or incorrect Handle.
 */
native BfReadShort(Handle:bf);

/**
 * Reads a 16bit unsigned integer from a readable bitbuffer (bf_read).
 *
 * @param bf			bf_read handle to read from.
 * @return				Integer value read (read as 16bit).
 * @error				Invalid or incorrect Handle.
 */
native BfReadWord(Handle:bf);

/**
 * Reads a normal integer to a readable bitbuffer (bf_read).
 *
 * @param bf			bf_read handle to read from.
 * @return				Integer value read (read as 32bit).
 * @error				Invalid or incorrect Handle.
 */
native BfReadNum(Handle:bf);

/**
 * Reads a floating point number from a readable bitbuffer (bf_read).
 *
 * @param bf			bf_read handle to read from.
 * @return				Floating point value read.
 * @error				Invalid or incorrect Handle.
 */
native Float:BfReadFloat(Handle:bf);

/**
 * Reads a string from a readable bitbuffer (bf_read).
 *
 * @param bf			bf_read handle to read from.
 * @param buffer		Destination string buffer.
 * @param maxlength		Maximum length of output string buffer.
 * @param line			If true the buffer will be copied until it reaches a '\n' or a null terminator.
 * @return				Number of bytes written to the buffer.  If the bitbuffer stream overflowed, 
 *						that is, had no terminator before the end of the stream, then a negative 
 *						number will be returned equal to the number of characters written to the 
 *						buffer minus 1.  The buffer will be null terminated regardless of the 
 *						return value.
 * @error				Invalid or incorrect Handle.
 */
native BfReadString(Handle:bf, String:buffer[], maxlength, bool:line=false);

/**
 * Reads an entity from a readable bitbuffer (bf_read).
 * @note This is a wrapper around BfReadShort().
 *
 * @param bf			bf_read handle to read from.
 * @return				Entity index read.
 * @error				Invalid or incorrect Handle.
 */
native BfReadEntity(Handle:bf);

/**
 * Reads a bit angle from a readable bitbuffer (bf_read).
 *
 * @param bf			bf_read handle to read from.
 * @param numBits		Optional number of bits to use.
 * @return				Angle read.
 * @error				Invalid or incorrect Handle.
 */
native Float:BfReadAngle(Handle:bf, numBits=8);

/**
 * Reads a coordinate from a readable bitbuffer (bf_read).
 *
 * @param bf			bf_read handle to read from.
 * @return				Coordinate read.
 * @error				Invalid or incorrect Handle.
 */
native Float:BfReadCoord(Handle:bf);

/**
 * Reads a 3D vector of coordinates from a readable bitbuffer (bf_read).
 *
 * @param bf			bf_read handle to read from.
 * @param coord			Destination coordinate array.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfReadVecCoord(Handle:bf, Float:coord[3]);

/**
 * Reads a 3D normal vector from a readable bitbuffer (bf_read).
 *
 * @param bf			bf_read handle to read from.
 * @param vec			Destination vector array.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfReadVecNormal(Handle:bf, Float:vec[3]);

/**
 * Reads a 3D angle vector from a readable bitbuffer (bf_read).
 *
 * @param bf			bf_read handle to read from.
 * @param angles		Destination angle vector.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfReadAngles(Handle:bf, Float:angles[3]);

/**
 * Returns the number of bytes left in a readable bitbuffer (bf_read).
 *
 * @param bf			bf_read handle to read from.
 * @return				Number of bytes left unread.
 * @error				Invalid or incorrect Handle.
 */
native BfGetNumBytesLeft(Handle:bf);
