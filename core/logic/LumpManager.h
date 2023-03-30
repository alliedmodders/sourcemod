/**
 * vim: set ts=4 :
 * =============================================================================
 * Entity Lump Manager
 * Copyright (C) 2021-2022 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_LUMPMANAGER_H_
#define _INCLUDE_LUMPMANAGER_H_

#include <vector>
#include <memory>
#include <string>

/**
 * Entity lump manager.  Provides a list that stores a list of key / value pairs and the
 * functionality to (de)serialize it from / to an entity string.
 * This file and its corresponding .cpp should be compilable independently of SourceMod;
 * the SourceMod interop is located within smn_entitylump.
 *
 * @file lumpmanager.h
 * @brief Class definition for object that parses lumps.
 */

/**
 * @brief A container of key / value pairs.
 */
using EntityLumpEntry = std::vector<std::pair<std::string, std::string>>;

enum EntityLumpParseStatus {
	Status_OK,
	Status_UnexpectedChar,
};

/**
 * @brief Result of parsing an entity lump.  On a parse error, m_Status is not Status_OK and
 * m_Position indicates the offset within the string that caused the parse error.
 */
struct EntityLumpParseResult {
	EntityLumpParseStatus m_Status;
	std::streamoff m_Position;
	
	operator bool() const;
	const char* Description() const;
};

/**
 * @brief Manages entity lump entries.
 */
class EntityLumpManager
{
public:
	/**
	 * @brief Parses the map entities string into an internal representation.
	 */
	EntityLumpParseResult Parse(const char* pMapEntities);
	
	/**
	 * @brief Dumps the current internal representation out to an std::string.
	 */
	std::string Dump();

	/**
	 * @brief Returns a weak reference to an EntityLumpEntry.  Used for handles on the scripting side.
	 */
	std::weak_ptr<EntityLumpEntry> Get(size_t index);

	/**
	 * @brief Removes an EntityLumpEntry at the given index, shifting down all entries after it by one.
	 */
	void Erase(size_t index);

	/**
	 * @brief Inserts a new EntityLumpEntry at the given index, shifting up the entries previously at the index and after it up by one.
	 */
	void Insert(size_t index);

	/**
	 * @brief Adds a new EntityLumpEntry to the end.  Returns the index of the entry.
	 */
	size_t Append();

	/**
	 * @brief Returns the number of EntityLumpEntry items in the list.
	 */
	size_t Length();

private:
	std::vector<std::shared_ptr<EntityLumpEntry>> m_Entities;
};

#endif // _INCLUDE_LUMPMANAGER_H_
