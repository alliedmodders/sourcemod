/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2007 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_CDATAPACK_H_
#define _INCLUDE_SOURCEMOD_CDATAPACK_H_

#include <IDataPack.h>

using namespace SourceMod;

class CDataPack : public IDataPack
{
public:
	CDataPack();
	~CDataPack();
public: //IDataReader
	void Reset() const;
	size_t GetPosition() const;
	bool SetPosition(size_t pos) const;
	cell_t ReadCell() const;
	float ReadFloat() const;
	bool IsReadable(size_t bytes) const;
	const char *ReadString(size_t *len) const;
	void *GetMemory() const;
	void *ReadMemory(size_t *size) const;
public: //IDataPack
	void ResetSize();
	void PackCell(cell_t cell);
	void PackFloat(float val);
	void PackString(const char *string);
	size_t CreateMemory(size_t size, void **addr);
public:
	void Initialize();
private:
	void CheckSize(size_t sizetype);
private:
	char *m_pBase;
	mutable char *m_curptr;
	size_t m_capacity;
	size_t m_size;
};

#endif //_INCLUDE_SOURCEMOD_CDATAPACK_H_
