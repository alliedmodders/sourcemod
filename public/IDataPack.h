/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
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

#ifndef _INCLUDE_SOURCEMOD_INTERFACE_DATAPACK_H_
#define _INCLUDE_SOURCEMOD_INTERFACE_DATAPACK_H_

#include <sp_vm_api.h>

/**
 * @file IDataPack.h
 * @brief Contains functions for packing data abstractly to/from plugins.  The wrappers
 * for creating these are contained in ISourceMod.h
 */

namespace SourceMod
{
	/**
	 * @brief Specifies a data pack that can only be read.
	 */
	class IDataReader
	{
	public:
		/**
		 * @brief Resets the position in the data stream to the beginning.
		 */
		virtual void Reset() const =0;

		/**
		 * @brief Retrieves the current stream position.
		 *
		 * @return			Index into the stream.
		 */
		virtual size_t GetPosition() const =0;

		/**
		 * @brief Sets the current stream position.
		 *
		 * @param pos		Index to set the stream at.
		 * @return			True if succeeded, false if out of bounds.
		 */
		virtual bool SetPosition(size_t pos) const =0;

		/**
		 * @brief Reads one cell from the data stream.
		 *
		 * @return			A cell read from the current position.
		 */
		virtual cell_t ReadCell() const =0;

		/**
		 * @brief Reads one float from the data stream.
		 *
		 * @return			A float read from the current position.
		 */
		virtual float ReadFloat() const =0;

		/**
		 * @brief Returns whether or not a specified number of bytes from the current stream
		 *  position to the end can be read.
		 * 
		 * @param bytes		Number of bytes to simulate reading.
		 * @return			True if can be read, false otherwise.
		 */
		virtual bool IsReadable(size_t bytes) const =0;

		/**
		 * @brief Reads a string from the data stream.
		 *
		 * @param len		Optional pointer to store the string length.
		 * @return			Pointer to the string, or NULL if out of bounds.
		 */
		virtual const char *ReadString(size_t *len) const =0;

		/**
		 * @brief Reads the current position as a generic address.
		 *
		 * @return			Pointer to the memory.
		 */
		virtual void *GetMemory() const =0;

		/**
		* @brief Reads the current position as a generic data type.
		*
		* @param size		Optional pointer to store the size of the data type.
		* @return			Pointer to the data, or NULL if out of bounds.
		*/
		virtual void *ReadMemory(size_t *size) const =0;

		/**
		 * @brief Reads a function pointer from the data stream.
		 *
		 * @return			A function pointer read from the current position.
		 */
		virtual cell_t ReadFunction() const =0;
	};

	/**
	 * @brief Specifies a data pack that can only be written.
	 */
	class IDataPack : public IDataReader
	{
	public:
		/**
		* @brief Resets the used size of the stream back to zero.
		*/
		virtual void ResetSize() =0;

		/**
		 * @brief Packs one cell into the data stream.
		 *
		 * @param cell		Cell value to write.
		 */
		virtual void PackCell(cell_t cell) =0;

		/**
		 * @brief Packs one float into the data stream.
		 *
		 * @param val		Float value to write.
		 */
		virtual void PackFloat(float val) =0;

		/**
		 * @brief Packs one string into the data stream.
		 * The length is recorded as well for buffer overrun protection.
		 *
		 * @param string	String to write.
		 */
		virtual void PackString(const char *string) =0;

		/**
		 * @brief Creates a generic block of memory in the stream.
		 *
		 * Note that the pointer it returns can be invalidated on further
		 * writing, since the stream size may grow.  You may need to double back
		 * and fetch the pointer again.
		 *
		 * @param size		Size of the memory to create in the stream.
		 * @param addr		Optional pointer to store the relocated memory address.
		 * @return			Current position of the stream beforehand.
		 */
		virtual size_t CreateMemory(size_t size, void **addr) =0;

		/**
		 * @brief Packs one function pointer into the data stream.
		 *
		 * @param function	The function pointer to write.
		 */
		virtual void PackFunction(cell_t function) =0;
	};
}

#endif //_INCLUDE_SOURCEMOD_INTERFACE_DATAPACK_H_
