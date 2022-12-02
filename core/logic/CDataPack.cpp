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

#include <memory>

#include "CDataPack.h"

CDataPack::CDataPack()
{
	Initialize();
}

CDataPack::~CDataPack()
{
	Initialize();
}

void CDataPack::Initialize()
{
	position = 0;
	
	do
	{
	} while (this->RemoveItem());

	elements.clear();
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
	elements.emplace(elements.begin() + position, val);

	return position++;
}

void CDataPack::PackCell(cell_t cell)
{
	InternalPack val;
	val.type = CDataPackType::Cell;
	val.pData.cval = cell;
	elements.emplace(elements.begin() + position, val);
	position++;
}

void CDataPack::PackFunction(cell_t function)
{
	InternalPack val;
	val.type = CDataPackType::Function;
	val.pData.cval = function;
	elements.emplace(elements.begin() + position, val);
	position++;
}

void CDataPack::PackFloat(float floatval)
{
	InternalPack val;
	val.type = CDataPackType::Float;
	val.pData.fval = floatval;
	elements.emplace(elements.begin() + position, val);
	position++;
}

void CDataPack::PackString(const char *string)
{
	InternalPack val;
	val.type = CDataPackType::String;
	std::string *sval = new std::string(string);
	val.pData.sval = sval;
	elements.emplace(elements.begin() + position, val);
	position++;
}

void CDataPack::PackCellArray(cell_t const *vals, cell_t count)
{
	InternalPack val;
	val.type = CDataPackType::CellArray;

	val.pData.aval = new cell_t [count + 1];
	memcpy(&val.pData.aval[1], vals, sizeof(cell_t) * count);
	val.pData.aval[0] = count;
	elements.emplace(elements.begin() + position, val);
	position++;
}

void CDataPack::PackFloatArray(cell_t const *vals, cell_t count)
{
	InternalPack val;
	val.type = CDataPackType::FloatArray;

	val.pData.aval = new cell_t [count + 1];
	memcpy(&val.pData.aval[1], vals, sizeof(cell_t) * count);
	val.pData.aval[0] = count;
	elements.emplace(elements.begin() + position, val);
	position++;
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
	if (pos > elements.size())
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
	return (position < elements.size());
}

const char *CDataPack::ReadString(size_t *len) const
{
	if (!IsReadable() || elements[position].type != CDataPackType::String)
	{
		if (len)
			*len = 0;

		return nullptr;
	}

	const std::string &val = *elements[position++].pData.sval;
	if (len)
		*len = val.size();

	return val.c_str();
}

cell_t *CDataPack::ReadCellArray(cell_t *size) const
{
	if (!IsReadable() || elements[position].type != CDataPackType::CellArray)
	{
		if(size)
			*size = 0;
		
		return nullptr;
	}

	cell_t *val = elements[position].pData.aval;
	cell_t *ptr = &(val[1]);
	++position;

	if (size)
		*size = val[0];

	return ptr;
}

cell_t *CDataPack::ReadFloatArray(cell_t *size) const
{
	if (!IsReadable() || elements[position].type != CDataPackType::FloatArray)
	{
		if(size)
			*size = 0;
		
		return nullptr;
	}

	cell_t *val = elements[position].pData.aval;
	cell_t *ptr = &(val[1]);
	++position;

	if (size)
		*size = val[0];

	return ptr;
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

bool CDataPack::RemoveItem(size_t pos)
{
	if (!elements.size())
	{
		return false;
	}

	if (pos == static_cast<size_t>(-1))
	{
		pos = position;
	}
	
	if (pos >= elements.size())
	{
		return false;
	}

	if (pos < position) // we're deleting under us, step back
	{
		--position;
	}

	switch (elements[pos].type)
	{
		case CDataPackType::Raw:
		{
			delete [] elements[pos].pData.vval;
			break;
		}

		case CDataPackType::String:
		{
			delete elements[pos].pData.sval;
			break;
		}

		case CDataPackType::CellArray:
		case CDataPackType::FloatArray:
		{
			delete [] elements[pos].pData.aval;
			break;
		}
	}

	elements.erase(elements.begin() + pos);
	return true;
}
