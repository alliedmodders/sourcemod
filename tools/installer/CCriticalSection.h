#ifndef _INCLUDE_INSTALLER_CRIT_SECT_H_
#define _INCLUDE_INSTALLER_CRIT_SECT_H_

#include "platform_headers.h"

class CCriticalSection
{
public:
	CCriticalSection()
	{
		InitializeCriticalSection(&m_crit);
	}
	~CCriticalSection()
	{
		DeleteCriticalSection(&m_crit);
	}
	void Enter()
	{
		EnterCriticalSection(&m_crit);
	}
	bool TryEnter()
	{
		if (TryEnterCriticalSection(&m_crit))
		{
			return true;
		}
		return false;
	}
	void Leave()
	{
		LeaveCriticalSection(&m_crit);
	}
private:
	CRITICAL_SECTION m_crit;
};

#endif //_INCLUDE_INSTALLER_CRIT_SECT_H_
