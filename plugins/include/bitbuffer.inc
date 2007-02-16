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
 * @param bif			bf_write handle to write to.
 * @param coord			Coordinate array to write.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteVecCoord(Handle:bf, Float:coord[3]);

/**
 * Writes a 3D normal vector to a writable bitbuffer (bf_write).
 *
 * @param bif			bf_write handle to write to.
 * @param vec			Vector to write.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteVecNormal(Handle:bf, Float:vec[3]);

/**
 * Writes a 3D angle vector to a writable bitbuffer (bf_write).
 *
 * @param bif			bf_write handle to write to.
 * @param angles		Angle vector to write.
 * @noreturn
 * @error				Invalid or incorrect Handle.
 */
native BfWriteAngles(Handle:bf, Float:angles[3]);
