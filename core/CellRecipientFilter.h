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

#ifndef _INCLUDE_SOURCEMOD_CELLRECIPIENTFILTER_H_
#define _INCLUDE_SOURCEMOD_CELLRECIPIENTFILTER_H_

#include <irecipientfilter.h>
#include <sp_vm_types.h>

class CellRecipientFilter : public IRecipientFilter
{
public:
	CellRecipientFilter() : m_IsReliable(false), m_IsInitMessage(false), m_Size(0) {}
	~CellRecipientFilter() {}
public: //IRecipientFilter
	bool IsReliable() const;
	bool IsInitMessage() const;
	int GetRecipientCount() const;
	int GetRecipientIndex(int slot) const;
public:
	void Initialize(const cell_t *ptr, size_t count);
	void SetToReliable(bool isreliable);
	void SetToInit(bool isinitmsg);
	void Reset();
private:
	cell_t m_Players[255];
	bool m_IsReliable;
	bool m_IsInitMessage;
	size_t m_Size;
};

inline void CellRecipientFilter::Reset()
{
	m_IsReliable = false;
	m_IsInitMessage = false;
	m_Size = 0;
}

inline bool CellRecipientFilter::IsReliable() const
{
	return m_IsReliable;
}

inline bool CellRecipientFilter::IsInitMessage() const
{
	return m_IsInitMessage;
}

inline int CellRecipientFilter::GetRecipientCount() const
{
	return m_Size;
}

inline int CellRecipientFilter::GetRecipientIndex(int slot) const
{
	int ret;
	if ((slot < 0) || (slot >= GetRecipientCount()))
	{
		ret = -1;
	}
	else
	{
		ret = static_cast<int>(m_Players[slot]);
	}

	return ret;
}

inline void CellRecipientFilter::SetToInit(bool isinitmsg)
{
	m_IsInitMessage = isinitmsg;
}

inline void CellRecipientFilter::SetToReliable(bool isreliable)
{
	m_IsReliable = isreliable;
}

inline void CellRecipientFilter::Initialize(const cell_t *ptr, size_t count)
{
	memcpy(m_Players, ptr, count * sizeof(cell_t));
	m_Size = count;
}

#endif //_INCLUDE_SOURCEMOD_CELLRECIPIENTFILTER_H_
