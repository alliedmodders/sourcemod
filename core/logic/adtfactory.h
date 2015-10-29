/**
 * vim: set ts=4 sw=4 tw=99 noet :
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

#ifndef _INCLUDE_SOURCEMOD_ADTFACTORY_H_
#define _INCLUDE_SOURCEMOD_ADTFACTORY_H_

#include <IADTFactory.h>
#include "common_logic.h"
#include <sm_stringhashmap.h>

using namespace SourceMod;

class BaseTrie : public IBasicTrie
{
public:
	BaseTrie();
	virtual ~BaseTrie();
	virtual bool Insert(const char *key, void *value);
	virtual bool Replace(const char *key, void *value);
	virtual bool Retrieve(const char *key, void **value);
	virtual bool Delete(const char *key);
	virtual void Clear();
	virtual void Destroy();
private:
	StringHashMap<void *> map_;
};

class ADTFactory : 
	public SMGlobalClass,
	public IADTFactory
{
public: //SMInterface
	const char *GetInterfaceName();
	unsigned int GetInterfaceVersion();
public: //SMGlobalClass
	void OnSourceModAllInitialized();
public: //IADTFactory
	IBasicTrie *CreateBasicTrie();
};

#endif //_INCLUDE_SOURCEMOD_ADTFACTORY_H_

