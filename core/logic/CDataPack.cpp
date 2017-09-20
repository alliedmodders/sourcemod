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
#include <amtl/am-string.h>

CDataPack::CDataPack()
{
	this->Initialize();
}

CDataPack::~CDataPack()
{
	this->Initialize();
}

static ke::Vector<ke::AutoPtr<CDataPack>> sDataPackCache;

IDataPack * CDataPack::New()
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
	for (size_t iter = 0; iter < this->elements.length(); ++iter)
	{
		switch (this->elements[iter].type)
		{
			case CDataPackType::Raw:
			{
				delete static_cast<uint8_t *>(this->elements[iter].pData.vval);
				this->elements.remove(iter--);
				break;
			}

			case CDataPackType::String:
			{
				delete static_cast<ke::AString *>(this->elements[iter].pData.vval);
				this->elements.remove(iter--);
				break;
			}
		}
	}

	this->elements.clear();
	this->position = 0;
}

void CDataPack::ResetSize()
{
	this->Initialize();
}

size_t CDataPack::CreateMemory(size_t size, void **addr)
{
	InternalPack val;
	val.type = CDataPackType::Raw;
	val.pData.vval = new uint8_t[size + 1];
	this->elements.insert(this->position, val);

	return this->position++;
}

void CDataPack::PackCell(cell_t cell)
{
	InternalPack val;
	val.type = CDataPackType::Cell;
	val.pData.cval = cell;
	this->elements.insert(this->position++, val);
}

void CDataPack::PackFunction(cell_t function)
{
	InternalPack val;
	val.type = CDataPackType::Function;
	val.pData.cval = function;
	this->elements.insert(this->position++, val);
}

void CDataPack::PackFloat(float floatval)
{
	InternalPack val;
	val.type = CDataPackType::Float;
	val.pData.fval = floatval;
	this->elements.insert(this->position++, val);
}

void CDataPack::PackString(const char *string)
{
	InternalPack val;
	val.type = CDataPackType::String;
	ke::AString *sval = new ke::AString(string);
	val.pData.vval = static_cast<void *>(sval);
	this->elements.insert(this->position++, val);
}

void CDataPack::Reset() const
{
	this->position = 0;
}

size_t CDataPack::GetPosition() const
{
	return this->position;
}

bool CDataPack::SetPosition(size_t pos) const
{
	if (pos > this->elements.length())
		return false;

	this->position = pos;
	return true;
}

cell_t CDataPack::ReadCell() const
{
	if (!this->IsReadable())
		return 0;

	if (this->elements[this->position].type != CDataPackType::Cell)
		return 0;
	
	return this->elements[this->position++].pData.cval;
}

cell_t CDataPack::ReadFunction() const
{
	if (!this->IsReadable())
		return 0;

	if (this->elements[this->position].type != CDataPackType::Function)
		return 0;
	
	return this->elements[this->position++].pData.cval;
}

float CDataPack::ReadFloat() const
{
	if (!this->IsReadable())
		return 0;

	if (this->elements[this->position].type != CDataPackType::Float)
		return 0;
	
	return this->elements[this->position++].pData.fval;
}

bool CDataPack::IsReadable(size_t bytes) const
{
	return (this->position < this->elements.length());
}

const char *CDataPack::ReadString(size_t *len) const
{
	if (!this->IsReadable())
	{
		if (len)
			*len = 0;

		return "";
	}

	if (this->elements[this->position].type != CDataPackType::String)
	{
		if (len)
			*len = 0;

		return "";
	}

	const ke::AString &val = *static_cast<ke::AString *>(this->elements[this->position++].pData.vval);
	if (len)
	{
		*len = val.length();
	}

	return val.chars();
}

void *CDataPack::GetMemory() const
{
	if (!this->IsReadable())
		return nullptr;
	
	if (this->elements[this->position].type != CDataPackType::Raw)
		return nullptr;

	return this->elements[this->position].pData.vval;
}

void *CDataPack::ReadMemory(size_t *size) const
{
	void *ptr = this->GetMemory();
	if (ptr != nullptr)
		++this->position;
	
	if (size)
		*size = (ptr != nullptr); /* Egor!!!! */

	return ptr;
}
