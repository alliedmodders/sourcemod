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

#ifndef _INCLUDE_VTABLE_HOOK_HELPER_H_
#define _INCLUDE_VTABLE_HOOK_HELPER_H_

#include <memory>
#include <thread>
#include <khook.hpp>

class CVTableHook
{
public:
	CVTableHook(void** vtable) : vtableptr(vtable)
	{
		this->vtableptr = vtable;
	}

	CVTableHook(const CVTableHook&) = delete;
	CVTableHook& operator= (const CVTableHook&) = delete;

	virtual ~CVTableHook() = default;
public:
	void** GetVTablePtr(void)
	{
		return this->vtableptr;
	}
private:
	void** vtableptr;
};

template<typename CONTEXT, typename CLASSNAME, typename RETURN, typename... ARGS>
class CVTableHookDetails : public CVTableHook {
public:
	using Self = CVTableHookDetails;

	CVTableHookDetails(void** vtable, std::int32_t index, CONTEXT* ctx, KHook::Return<RETURN> (CONTEXT::*pre)(CLASSNAME*, ARGS...), KHook::Return<RETURN> (CONTEXT::*post)(CLASSNAME*, ARGS...))
	: CVTableHook(vtable),
	_ctx(ctx),
	_main_thread(std::this_thread::get_id()),
	_pre(pre),
	_post(post) {
		_id = KHook::SetupVirtualHook(
			vtable,
			index,
			this,
			nullptr,
			KHook::ExtractMFP(&Self::_KHOOK_CALLBACK_PRE),
			KHook::ExtractMFP(&Self::_KHOOK_CALLBACK_POST),
			KHook::ExtractMFP(&Self::_KHOOK_MAKE_RETURN),
			KHook::ExtractMFP(&Self::_KHOOK_MAKE_ORIGINAL),
			KHook::Hook<RETURN>::template _copy_stack_size<void*, ARGS...>(),
			false);
	}

	virtual ~CVTableHookDetails() {
		if (_id != KHook::INVALID_HOOK) {
			KHook::RemoveHook(_id, true);
		}
	}

	inline RETURN _KHOOK_CALLBACK(bool pre, CLASSNAME* original_this, ARGS... args) {
		if (std::this_thread::get_id() != _main_thread) {
			if constexpr(!std::is_same<RETURN, void>::value) {
				KHook::SaveReturnValue(KHook::Action::Ignore, nullptr, 0, nullptr, nullptr, false);
				return RETURN();
			} else {
				return;
			}
		}

		auto ret = (pre) ? (_ctx->*_pre)(original_this, args...) : (_ctx->*_post)(original_this, args...);

		RETURN* return_ptr = nullptr;
		void* init_op = nullptr;
		void* deinit_op = nullptr;
		std::size_t size = 0;
		if constexpr(!std::is_same<RETURN, void>::value) {	
			return_ptr = const_cast<RETURN*>(&ret.ret);
			init_op = reinterpret_cast<void*>(::KHook::init_operator<RETURN>);
			deinit_op = reinterpret_cast<void*>(::KHook::deinit_operator<RETURN>);
			size = sizeof(RETURN);
		}

		::KHook::SaveReturnValue(ret.action, return_ptr, size, init_op, deinit_op, false);
		if constexpr(!std::is_same<RETURN, void>::value) {
			return *return_ptr;
		} else {
			return;
		}
	}

	RETURN _KHOOK_CALLBACK_PRE(ARGS... args) {
		auto real_this = (Self*)KHook::GetContext();
		return real_this->_KHOOK_CALLBACK(true, (CLASSNAME*)this, args...);
	}

	RETURN _KHOOK_CALLBACK_POST(ARGS... args) {
		auto real_this = (Self*)KHook::GetContext();
		return real_this->_KHOOK_CALLBACK(false, (CLASSNAME*)this, args...);
	}

	RETURN _KHOOK_MAKE_RETURN(ARGS...) {
		if constexpr(std::is_same<RETURN, void>::value) {
			::KHook::DestroyReturnValue();
			return;
		} else {
			RETURN ret = *(RETURN*)::KHook::GetCurrentValuePtr(true);
			::KHook::DestroyReturnValue();
			return ret;
		}
	}

	RETURN _KHOOK_MAKE_ORIGINAL(ARGS ...args) {
		auto ptr = ::KHook::BuildMFP<RETURN (CLASSNAME::*)(ARGS...)>(::KHook::GetOriginalFunction());
		if constexpr(std::is_same<RETURN, void>::value) {
			(((CLASSNAME*)this)->*ptr)(args...);
			::KHook::__internal__savereturnvalue(KHook::Return<void>{ KHook::Action::Ignore }, true);
		} else {
			RETURN ret = (((CLASSNAME*)this)->*ptr)(args...);
			::KHook::__internal__savereturnvalue(KHook::Return<RETURN>{ KHook::Action::Ignore, ret }, true);
			return ret;
		}
	}

	KHook::HookID_t _id;
	CONTEXT* _ctx;
	std::thread::id _main_thread;
	KHook::Return<RETURN> (CONTEXT::*_pre)(CLASSNAME*, ARGS...);
	KHook::Return<RETURN> (CONTEXT::*_post)(CLASSNAME*, ARGS...);
};

#endif //_INCLUDE_VTABLE_HOOK_HELPER_H_
