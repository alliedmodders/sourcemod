/**
 * vim: set ts=4 sw=4 tw=99 noet :
 * =============================================================================
 * SourceMod
 * Copyright (C) 2004-2014 AlliedModders LLC.  All rights reserved.
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
#ifndef _include_sourcemod_handle_helpers_h_
#define _include_sourcemod_handle_helpers_h_

#include "common_logic.h"
#include <IHandleSys.h>

// T must be a pointer type.
template <typename T>
class OpenHandle
{
public:
	OpenHandle()
	: t_(),
	  error_(HandleError_None)
	{}
	explicit OpenHandle(T *t)
	: t_(t),
	  error_(HandleError_None)
	{}
	OpenHandle(const OpenHandle &other)
	: t_(other.t_),
	  error_(other.error_)
	{}
	OpenHandle(IPluginContext *cx, Handle_t handle, HandleType_t type)
	: t_(NULL)
	{
		HandleSecurity sec(cx->GetIdentity(), g_pCoreIdent);
		error_ = handlesys->ReadHandle(handle, type, &sec, (void **)&t_);
		if (error_ != HandleError_None)
			cx->ThrowNativeError("invalid handle %x (error: %d)", handle, error_);
	}

	bool Ok() const {
		return t_ && error_ == HandleError_None;
	}

	operator T *() const {
		assert(Ok());
		return t_;
	}
	T *operator *() const {
		assert(Ok());
		return t_;
	}
	T *operator ->() const {
		assert(Ok());
		return t_;
	}

	OpenHandle &operator =(T *t) {
		t_ = t;
		error_ = HandleError_None;
		return *this;
	}
	OpenHandle &operator =(const OpenHandle &other) {
		t_ = other.t_;
		error_ = other.error_;
		return *this;
	}

private:
	T *t_;
	HandleError error_;
};

#endif // _include_sourcemod_handle_helpers_h_
