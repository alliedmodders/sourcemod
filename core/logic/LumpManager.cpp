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

#include "LumpManager.h"

#include <iomanip>
#include <sstream>

EntityLumpParseResult::operator bool() const {
	return m_Status == Status_OK;
}

EntityLumpParseResult EntityLumpManager::Parse(const char* pMapEntities) {
	m_Entities.clear();
	
	std::istringstream mapEntities(pMapEntities);
	
	for (;;) {
		std::string token;
		mapEntities >> std::ws >> token >> std::ws;
		
		// Assert that we're at the start of a new block, otherwise we're done parsing
		if (token != "{") {
			if (token == "\0") {
				break;
			} else {
				return EntityLumpParseResult {
					Status_UnexpectedChar, mapEntities.tellg()
				};
			}
		}
		
		/**
		 * Parse key / value pairs until we reach a closing brace.  We currently assume there
		 * are only quoted keys / values up to the next closing brace.
		 *
		 * The SDK suggests that there are cases that could use non-quoted symbols and nested
		 * braces (`shared/mapentities_shared.cpp::MapEntity_ParseToken`), but I haven't seen
		 * those in practice.
		 */
		EntityLumpEntry entry;
		while (mapEntities.peek() != '}') {
			std::string key, value;
			
			if (mapEntities.peek() != '"') {
				return EntityLumpParseResult {
					Status_UnexpectedChar, mapEntities.tellg()
				};
			}
			mapEntities >> quoted(key) >> std::ws;
			
			if (mapEntities.peek() != '"') {
				return EntityLumpParseResult {
					Status_UnexpectedChar, mapEntities.tellg()
				};
			}
			mapEntities >> quoted(value) >> std::ws;
			
			entry.emplace_back(key, value);
		}
		mapEntities.get();
		m_Entities.push_back(std::make_shared<EntityLumpEntry>(entry));
	}
	
	return EntityLumpParseResult{};
}

std::string EntityLumpManager::Dump() {
	std::ostringstream stream;
	for (const auto& entry : m_Entities) {
		// ignore empty entries
		if (entry->empty()) {
			continue;
		}
		stream << "{\n";
		for (const auto& pair : *entry) {
			stream << '"' << pair.first << "\" \"" << pair.second << '"' << '\n';
		}
		stream << "}\n";
	}
	return stream.str();
}

std::weak_ptr<EntityLumpEntry> EntityLumpManager::Get(size_t index) {
	return m_Entities[index];
}

void EntityLumpManager::Erase(size_t index) {
	m_Entities.erase(m_Entities.begin() + index);
}

void EntityLumpManager::Insert(size_t index) {
	m_Entities.emplace(m_Entities.begin() + index, std::make_shared<EntityLumpEntry>());
}

size_t EntityLumpManager::Append() {
	auto it = m_Entities.emplace(m_Entities.end(), std::make_shared<EntityLumpEntry>());
	return std::distance(m_Entities.begin(), it);
}

size_t EntityLumpManager::Length() {
	return m_Entities.size();
}
