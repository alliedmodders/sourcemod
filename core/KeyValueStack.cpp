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

#include "KeyValueStack.h"
#include "utlbuffer.h"
#include <KeyValues.h>

KeyValueStack::KeyValueStack(KeyValues *root)
{
	pRoot = root;
}

KeyValueStack::~KeyValueStack()
{
	if (m_bDeleteOnDestroy)
	{
		if (pRoot)
		{
			GetRootSection()->deleteThis();
		}
	}
}

KeyValues *KeyValueStack::GetRootSection()
{
	return pRoot;
}

KeyValues *KeyValueStack::GetCurrentSection()
{
	if (!pCurrentPath.empty())
	{
		return pCurrentPath.front();
	}
	return GetRootSection();
}

bool KeyValueStack::JumpToKey(const char * keyName, bool create)
{
	KeyValues *pJumpTo = GetCurrentSection()->FindKey(keyName, create);
	if (!pJumpTo)
	{
		return false;
	}
	pCurrentPath.push(pJumpTo);
	return true;
}

bool KeyValueStack::JumpToKeySymbol(int symbol)
{
	KeyValues *pJumpTo = GetCurrentSection()->FindKey(symbol);
	if (!pJumpTo)
	{
		return false;
	}
	pCurrentPath.push(pJumpTo);
	return true;
}

bool KeyValueStack::GotoNextKey(bool keyOnly)
{
	if (pCurrentPath.empty())
	{
		// there should only be one root section, no next keys exist
		return false;
	}
	KeyValues *pCurrentSection = GetCurrentSection();
	KeyValues *pNextKey;
	if (keyOnly)
	{
		// not realy sure why ``GetNextTrueSubKey`` is not supposed
		// add another level lo the stack, but w/e
		pNextKey = pCurrentSection->GetNextTrueSubKey();
	}
	else
	{
		pNextKey = pCurrentSection->GetNextKey();
	}

	if (!pNextKey)
	{
		return false;
	}
	pCurrentPath.pop();
	pCurrentPath.push(pNextKey);

	return true;
}

bool KeyValueStack::GotoFirstSubKey(bool keyOnly)
{
	KeyValues *pCurrentSection = GetCurrentSection();
	KeyValues *pFirstSubKey;
	if (keyOnly)
	{
		pFirstSubKey = pCurrentSection->GetFirstTrueSubKey();
	}
	else
	{
		pFirstSubKey = pCurrentSection->GetFirstSubKey();
	}

	if (!pFirstSubKey)
	{
		return false;
	}
	pCurrentPath.push(pFirstSubKey);

	return true;
}

bool KeyValueStack::SavePosition()
{
	if (pCurrentPath.empty())
	{
		// there's no point on saving the root position if it's already there
		// and there's only one root key
		return false;
	}

	pCurrentPath.push(pCurrentPath.front());
	return true;
}

bool KeyValueStack::GoBack()
{
	if (pCurrentPath.empty())
	{
		return false;
	}
	pCurrentPath.pop();
	return true;
}

KeyValueStack::DeleteThis_Result KeyValueStack::DeleteThis()
{
	if (pCurrentPath.empty())
	{
		return DeleteThis_Failed;
	}
	KeyValues *pToDelete = GetCurrentSection();
	KeyValues *pPathCandidate = pCurrentPath.second();

	KeyValues *pSubPathCandidate = pPathCandidate->GetFirstSubKey();
	while (pSubPathCandidate)
	{
		if (pSubPathCandidate == pToDelete)
		{
			KeyValues *pJumpTo = pSubPathCandidate->GetNextKey();
			pPathCandidate->RemoveSubKey(pToDelete);
			pCurrentPath.pop();
			pToDelete->deleteThis();
			if (pJumpTo)
			{
				pCurrentPath.push(pJumpTo);
				return DeleteThis_MovedNext;
			}
			else
			{
				return DeleteThis_MovedBack;
			}
		}
		pSubPathCandidate = pSubPathCandidate->GetNextKey();
	}

	return DeleteThis_Failed;
}

void KeyValueStack::Rewind()
{
	pCurrentPath.popall();
}

size_t KeyValueStack::GetNodeCount()
{
	return pCurrentPath.size();
}

size_t KeyValueStack::CalcSize()
{
	return sizeof(*this) + pCurrentPath.size() * sizeof(KeyValues *) + CalcKVSize();
}

bool KeyValueStack::DeleteKVOnDestroy()
{
	return m_bDeleteOnDestroy;
}

void KeyValueStack::SetDeleteKVOnDestroy(bool deleteOnDestroy)
{
	m_bDeleteOnDestroy = deleteOnDestroy;
}

size_t KeyValueStack::CalcKVSize()
{
	CUtlBuffer buf;
	size_t size;

	GetRootSection()->RecursiveSaveToFile(buf, 0);
	size = buf.TellMaxPut();

	buf.Purge();

	return size;
}
