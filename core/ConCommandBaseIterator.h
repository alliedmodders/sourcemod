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
#ifndef _INCLUDE_SOURCEMOD_CON_COMMAND_BASE_ITERATOR_H_
#define _INCLUDE_SOURCEMOD_CON_COMMAND_BASE_ITERATOR_H_

class ConCommandBaseIterator
{
#if (SOURCE_ENGINE == SE_LEFT4DEAD) || (SOURCE_ENGINE == SE_LEFT4DEAD2)
	ICvarIteratorInternal *cvarIter;
#else
	ConCommandBase *cvarIter;
#endif

public:
	ConCommandBaseIterator()
	{
#if (SOURCE_ENGINE == SE_LEFT4DEAD) || (SOURCE_ENGINE == SE_LEFT4DEAD2)
		cvarIter = icvar->FactoryInternalIterator();
		cvarIter->SetFirst();
#else
		cvarIter = icvar->GetCommands();
#endif
	}
	
	~ConCommandBaseIterator()
	{
#if (SOURCE_ENGINE == SE_LEFT4DEAD) || (SOURCE_ENGINE == SE_LEFT4DEAD2)
		g_pMemAlloc->Free(cvarIter);
#endif
	}

	inline bool IsValid()
	{
#if (SOURCE_ENGINE == SE_LEFT4DEAD) || (SOURCE_ENGINE == SE_LEFT4DEAD2)
		return cvarIter->IsValid();
#else
		return cvarIter != NULL;
#endif
	}

	inline void Next()
	{
#if (SOURCE_ENGINE == SE_LEFT4DEAD) || (SOURCE_ENGINE == SE_LEFT4DEAD2)
		cvarIter->Next();
#else
		cvarIter = const_cast<ConCommandBase*>(cvarIter->GetNext());
#endif
	}

	inline ConCommandBase *Get()
	{
#if (SOURCE_ENGINE == SE_LEFT4DEAD) || (SOURCE_ENGINE == SE_LEFT4DEAD2)
		return cvarIter->Get();
#else
		return cvarIter;
#endif
	}
};

#endif /* _INCLUDE_SOURCEMOD_CON_COMMAND_BASE_ITERATOR_H_ */

