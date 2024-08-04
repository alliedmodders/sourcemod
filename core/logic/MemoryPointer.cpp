/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2024 AlliedModders LLC.  All rights reserved.
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

#include "MemoryPointer.h"
#include <sourcehook.h>
#include <sh_memory.h>
#include <algorithm>

MemoryPointer::MemoryPointer(cell_t size) : m_ptr(malloc(size)), m_owned(true), m_size(size)
{
}

MemoryPointer::MemoryPointer(void* ptr, cell_t size) : m_ptr(ptr), m_owned(false), m_size(size)
{
}

MemoryPointer::~MemoryPointer()
{
	if (m_owned && m_ptr)
	{
		free(m_ptr);
		m_ptr = nullptr;
	}
}

void MemoryPointer::Delete()
{
	delete this;
}

void* MemoryPointer::Get()
{
	return m_ptr;
}

cell_t MemoryPointer::GetSize()
{
	return m_size;
}