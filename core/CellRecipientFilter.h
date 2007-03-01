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
	CellRecipientFilter() : m_Reliable(false), m_InitMessage(false), m_Size(0) {}
	~CellRecipientFilter() {}
public: //IRecipientFilter
	bool IsReliable() const;
	bool IsInitMessage() const;
	int GetRecipientCount() const;
	int GetRecipientIndex(int slot) const;
public:
	void SetRecipientPtr(cell_t *ptr, size_t count);
	void SetReliable(bool isreliable);
	void SetInitMessage(bool isinitmsg);
	void ResetFilter();
private:
	cell_t m_CellRecipients[255];
	bool m_Reliable;
	bool m_InitMessage;
	size_t m_Size;
};

inline void CellRecipientFilter::ResetFilter()
{
	m_Reliable = false;
	m_InitMessage = false;
	m_Size = 0;
}

inline bool CellRecipientFilter::IsReliable() const
{
	return m_Reliable;
}

inline bool CellRecipientFilter::IsInitMessage() const
{
	return m_InitMessage;
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
	return static_cast<int>(m_CellRecipients[slot]);
}

inline void CellRecipientFilter::SetInitMessage(bool isinitmsg)
{
	m_InitMessage = isinitmsg;
}

inline void CellRecipientFilter::SetReliable(bool isreliable)
{
	m_Reliable = isreliable;
}

inline void CellRecipientFilter::SetRecipientPtr(cell_t *ptr, size_t count)
{
	memcpy(m_CellRecipients, ptr, count * sizeof(cell_t));
	m_Size = count;
}

#endif //_INCLUDE_SOURCEMOD_CELLRECIPIENTFILTER_H_
