/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod BinTools Extension
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
 * Version: $Id: CallMaker.h 1964 2008-03-27 04:54:56Z damagedsoul $
 */

#ifndef _INCLUDE_SOURCEMOD_HOOKWRAPPER_H_
#define _INCLUDE_SOURCEMOD_HOOKWRAPPER_H_

#if defined HOOKING_ENABLED

#include "smsdk_ext.h"
#include "sourcehook.h"
#include "IBinTools.h"

#define VTABLE_PATCH_OFFS		2

using namespace SourceMod;

class HookWrapper : public IHookWrapper
{
public:
	HookWrapper(SourceHook::ISourceHook *pSH, 
				const SourceHook::ProtoInfo *proto, 
				SourceHook::MemFuncInfo *memfunc, 
				void *addr
				);
	~HookWrapper();
public: //IHookWrapper
	ISMDelegate *CreateDelegate(void *data);
	unsigned int GetParamCount();
	unsigned int GetParamOffset(unsigned int argnum, unsigned int *size);
	void PerformRecall(void *params, void *retval);
	void Destroy();
public:
	unsigned int GetParamSize();
	unsigned int GetRetSize();
	SourceHook::ProtoInfo *GetProtoInfo();
	void *GetHandlerAddr();
	void SetHookWrpAddr(void *addr);
	void SetCallWrapperAddr(ICallWrapper *wrap);
private:
	void *m_FuncAddr;
	void *m_HookWrapper;
	SourceHook::ISourceHook *m_pSH;
	unsigned int *m_ParamOffs;
	unsigned int m_ParamSize;
	unsigned int m_RetSize;
	SourceHook::MemFuncInfo m_MemFuncInfo;
	SourceHook::ProtoInfo m_ProtoInfo;
	ICallWrapper *m_CallWrapper;
};

class SMDelegate : public ISMDelegate
{
public:
	SMDelegate(void *data);
public: //SourceHook::ISHDelegate
	bool IsEqual(ISHDelegate *pOtherDeleg);
	void DeleteThis();
	void Call();
public: //ISMDelegate
	void *GetUserData();
public:
	void PatchVtable(void *addr);
private:
	void *m_Data;
};

#endif

#endif //_INCLUDE_SOURCEMOD_HOOKWRAPPER_H_
