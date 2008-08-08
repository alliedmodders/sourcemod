/**
* vim: set ts=4 :
* =============================================================================
* SourceMod Sample Extension
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

#ifndef _INCLUDE_SOURCEMOD_ISTRUCTABSTACTER_H
#define _INCLUDE_SOURCEMOD_ISTRUCTABSTACTER_H

#include <IShareSys.h>
#include <IHandleSys.h>

using namespace SourceMod;

#define SMINTERFACE_ISTRUCTABSTACTER_NAME	"IStructAbstacter"
#define SMINTERFACE_ISTRUCTABSTACTER_VERSION	1

class IStructAbstracter : public SourceMod::SMInterface
{
public:
	virtual const char *GetInterfaceName()
	{
		return SMINTERFACE_ISTRUCTABSTACTER_NAME;
	}
	virtual unsigned int GetInterfaceVersion()
	{
		return SMINTERFACE_ISTRUCTABSTACTER_VERSION;
	}

public:
	virtual Handle_t CreateStructHandle(const char *type, void *str) =0;
	//virtual void SetInitializer(char *type, STRUCT_INITIALIZER init);

};

#endif //_INCLUDE_SOURCEMOD_ISTRUCTABSTACTER_H
