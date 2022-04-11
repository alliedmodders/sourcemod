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

#include <IHandleSys.h>
#include "sm_globals.h"

using namespace SourceMod;

/**
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
class EntityLumpManager :
		public SMGlobalClass,
		public IHandleTypeDispatch
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

	void Erase(size_t index);

	void Insert(size_t index);

	size_t Append();

	size_t Length();

public: //IHandleTypeDispatch
	void OnHandleDestroy(HandleType_t type, void* object);

public: //SMGlobalClass
	void OnSourceModAllInitialized();
	void OnSourceModLevelChange(const char *mapName);
	void OnSourceModShutdown();

private:
	std::vector<std::shared_ptr<EntityLumpEntry>> m_Entities;
};

extern EntityLumpManager *lumpmanager;
extern std::string g_strMapEntities;
extern bool g_bLumpAvailableForWriting;

extern HandleType_t g_EntityLumpEntryType;

#endif // _INCLUDE_SOURCEMOD_EXTENSION_PROPER_H_
