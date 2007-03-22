/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod (C)2004-2007 AlliedModders LLC.  All rights reserved.
 * ===============================================================
 *
 * This file is not open source and may not be copied without explicit
 * written permission of AlliedModders LLC.  This file may not be redistributed 
 * in whole or significant part.
 * For information, see LICENSE.txt or http://www.sourcemod.net/license.php
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
	void Initialize(cell_t *ptr, size_t count);
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
	if ((slot < 0) || (slot >= GetRecipientCount()))
	{
		return -1;
	}
	return static_cast<int>(m_Players[slot]);
}

inline void CellRecipientFilter::SetToInit(bool isinitmsg)
{
	m_IsInitMessage = isinitmsg;
}

inline void CellRecipientFilter::SetToReliable(bool isreliable)
{
	m_IsReliable = isreliable;
}

inline void CellRecipientFilter::Initialize(cell_t *ptr, size_t count)
{
	memcpy(m_Players, ptr, count * sizeof(cell_t));
	m_Size = count;
}

#endif //_INCLUDE_SOURCEMOD_CELLRECIPIENTFILTER_H_
