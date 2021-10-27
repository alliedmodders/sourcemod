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
#ifndef _include_sourcemod_native_h_
#define _include_sourcemod_native_h_

#include <IShareSys.h>
#include <IHandleSys.h>
#include <am-string.h>
#include <am-utility.h>
#include <am-refcounting.h>
#include <sm_stringhashmap.h>
#include "common_logic.h"

class CNativeOwner;

struct FakeNative
{
	FakeNative(const char *name, IPluginFunction *fun)
		: name(name),
		  ctx(fun->GetParentContext()),
		  call(fun)
	{
	}

	std::string name;
	IPluginContext *ctx;
	IPluginFunction *call;
	ke::RefPtr<INativeCallback> wrapper;
};

struct Native : public ke::Refcounted<Native>
{
	Native(CNativeOwner *owner, const sp_nativeinfo_t *native)
		: owner(owner),
		  native(native),
		  fake(nullptr)
	{
	}
	Native(CNativeOwner *owner, std::unique_ptr<FakeNative>&& fake)
		: owner(owner),
		  native(nullptr),
		  fake(std::move(fake))
	{
	}

	CNativeOwner *owner;
	const sp_nativeinfo_t *native;
	std::unique_ptr<FakeNative> fake;

	const char *name() const
	{
		if (native)
			return native->name;
		return fake->name.c_str();
	}

	static inline bool matches(const char *name, const ke::RefPtr<Native> &entry)
	{
		return strcmp(name, entry->name()) == 0;
	}
	static inline uint32_t hash(const detail::CharsAndLength &key)
	{
		return key.hash();
	}
};


#endif // _include_sourcemod_native_h_

