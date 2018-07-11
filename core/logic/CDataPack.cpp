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

#include <stdlib.h>
#include <string.h>
#include "CDataPack.h"
#include <amtl/am-autoptr.h>

CDataPack::CDataPack()
{
	Initialize();
}

CDataPack::~CDataPack()
{
	Initialize();
}

static ke::Vector<ke::AutoPtr<CDataPack>> sDataPackCache;

IDataPack *CDataPack::New()
{
  if (sDataPackCache.empty())
    return new CDataPack();

  CDataPack *pack = sDataPackCache.back().take();
  sDataPackCache.pop();
  pack->Initialize();
  return pack;
}

void
CDataPack::Free(IDataPack *pack)
{
  sDataPackCache.append(static_cast<CDataPack *>(pack));
}

void CDataPack::Initialize()
{
	for (size_t index = 0; index < elements.length(); ++index)
	{
		switch (elements[index].type)
		{
			case CDataPackType::Raw:
			{
				delete elements[index].pData.vval;
				elements.remove(index--);
				break;
			}

			case CDataPackType::String:
			{
				delete elements[index].pData.sval;
				elements.remove(index--);
				break;
			}
		}
	}

	elements.clear();
	position = 0;
}

void CDataPack::ResetSize()
{
	Initialize();
}

size_t CDataPack::CreateMemory(size_t size, void **addr)
{
	InternalPack val;
	val.type = CDataPackType::Raw;
	val.pData.vval = new uint8_t[size + sizeof(size)];
	reinterpret_cast<size_t *>(val.pData.vval)[0] = size;
	elements.insert(position, val);

	return position++;
}

void CDataPack::PackCell(cell_t cell)
{
	InternalPack val;
	val.type = CDataPackType::Cell;
	val.pData.cval = cell;
	elements.insert(position++, val);
}

void CDataPack::PackFunction(cell_t function)
{
	InternalPack val;
	val.type = CDataPackType::Function;
	val.pData.cval = function;
	elements.insert(position++, val);
}

void CDataPack::PackFloat(float floatval)
{
	InternalPack val;
	val.type = CDataPackType::Float;
	val.pData.fval = floatval;
	elements.insert(position++, val);
}

void CDataPack::PackString(const char *string)
{
	InternalPack val;
	val.type = CDataPackType::String;
	ke::AString *sval = new ke::AString(string);
	val.pData.sval = sval;
	elements.insert(position++, val);
}

void CDataPack::Reset() const
{
	position = 0;
}

size_t CDataPack::GetPosition() const
{
	return position;
}

bool CDataPack::SetPosition(size_t pos) const
{
	if (pos > elements.length())
		return false;

	position = pos;
	return true;
}

cell_t CDataPack::ReadCell() const
{
	if (!IsReadable() || elements[position].type != CDataPackType::Cell)
		return 0;
	
	return elements[position++].pData.cval;
}

cell_t CDataPack::ReadFunction() const
{
	if (!IsReadable() || elements[position].type != CDataPackType::Function)
		return 0;
	
	return elements[position++].pData.cval;
}

float CDataPack::ReadFloat() const
{
	if (!IsReadable() || elements[position].type != CDataPackType::Float)
		return 0;
	
	return elements[position++].pData.fval;
}

bool CDataPack::IsReadable(size_t bytes) const
{
	return (position < elements.length());
}

const char *CDataPack::ReadString(size_t *len) const
{
	if (!IsReadable() || elements[position].type != CDataPackType::String)
	{
		if (len)
			*len = 0;

		return nullptr;
	}

	const ke::AString &val = *elements[position++].pData.sval;
	if (len)
		*len = val.length();

	return val.chars();
}

void *CDataPack::GetMemory() const
{
	return nullptr;
}

void *CDataPack::ReadMemory(size_t *size) const
{
	void *ptr = nullptr;
	if (!IsReadable() || elements[position].type != CDataPackType::Raw)
		return ptr;

	size_t *val = reinterpret_cast<size_t *>(elements[position].pData.vval);
	ptr = &(val[1]);
	++position;

	if (size)
		*size = val[0]; /* Egor!!!! */

	return ptr;
}

void CDataPack::RemoveItem(size_t pos)
{
	if (pos < 0)
	{
		elements.remove(position--);
		return;
	}
	
	assert(pos > 0 && pos <= elements.length());
	elements.remove(pos-1);
	--position;
}
