/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_AUTO_HANDLE_ROOTER_H_
#define _INCLUDE_SOURCEMOD_AUTO_HANDLE_ROOTER_H_

#include <IHandleSys.h>

class AutoHandleRooter
{
public:
	AutoHandleRooter(Handle_t hndl)
	{
		if (hndl != BAD_HANDLE)
			this->hndl = handlesys->FastCloneHandle(hndl);
		else
			this->hndl = BAD_HANDLE;
	}

	~AutoHandleRooter()
	{
		if (hndl != BAD_HANDLE)
		{
			HandleSecurity sec(g_pCoreIdent, g_pCoreIdent);
			handlesys->FreeHandle(hndl, &sec);
		}
	}

	Handle_t getHandle()
	{
		return hndl;
	}
private:
	Handle_t hndl;
};

class AutoHandleCloner
{
	Handle_t original;
	HandleSecurity sec;
	AutoHandleRooter ahr;
public:
	AutoHandleCloner(Handle_t original, HandleSecurity sec)
	  : original(original), sec(sec), ahr(original)
	{
	}

	~AutoHandleCloner()
	{
		if (original != BAD_HANDLE)
		{
			handlesys->FreeHandle(original, &sec);
		}
	}

	Handle_t getClone()
	{
		return ahr.getHandle();
	}
};

class AutoHandleIdentLocker
{
public:
	AutoHandleIdentLocker() : pSecurity(nullptr)
	{
	}

	AutoHandleIdentLocker(Handle_t hndl) : pSecurity(nullptr)
	{
		if (hndl != BAD_HANDLE)
		{
			if (handlesys->GetHandleAccess(hndl, this->pSecurity) == HandleError_None)
			{
				if ((this->pSecurity->access[HandleAccess_Delete] & HANDLE_RESTRICT_IDENTEXCLUSIVE) == HANDLE_RESTRICT_IDENTEXCLUSIVE)
					this->pSecurity = nullptr;
				else
					this->pSecurity->access[HandleAccess_Delete] |= HANDLE_RESTRICT_IDENTEXCLUSIVE;
			}
		}
	}

	~AutoHandleIdentLocker()
	{
		this->Nuke();
	}

public:
	AutoHandleIdentLocker &operator =(const AutoHandleIdentLocker &other)
	{
		this->Nuke();
		this->pSecurity = other.pSecurity;
		return *this;
	};
private:
	void Nuke(void)
	{
		if (this->pSecurity)
		{
			this->pSecurity->access[HandleAccess_Delete] &= ~HANDLE_RESTRICT_IDENTEXCLUSIVE;
			this->pSecurity = nullptr;
		}
	}
private:
	HandleAccess *pSecurity;
};

#endif /* _INCLUDE_SOURCEMOD_AUTO_HANDLE_ROOTER_H_ */

