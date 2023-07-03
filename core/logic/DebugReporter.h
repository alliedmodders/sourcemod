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

#ifndef _INCLUDE_SOURCEMOD_CDBGREPORTER_H_
#define _INCLUDE_SOURCEMOD_CDBGREPORTER_H_

#include "sp_vm_api.h"
#include "common_logic.h"
#include <am-vector.h>
#include <am-string.h>

class DebugReport : 
	public SMGlobalClass, 
	public IDebugListener
{
public: // SMGlobalClass
	void OnSourceModAllInitialized();
public: // IDebugListener
	void ReportError(const IErrorReport &report, IFrameIterator &iter);
	void OnDebugSpew(const char *msg, ...);
public:
	// If err is -1, caller assumes the automatic reporting by SourcePawn is
	// good enough, and only wants the supplemental logging provided here.
	void GenerateError(IPluginContext *ctx, cell_t func_idx, int err, const char *message, ...);
	void GenerateErrorVA(IPluginContext *ctx, cell_t func_idx, int err, const char *message, va_list ap); 
	void GenerateCodeError(IPluginContext *ctx, uint32_t code_addr, int err, const char *message, ...);
	std::vector<std::string> GetStackTrace(IFrameIterator *iter);
private:
	int _GetPluginIndex(IPluginContext *ctx);
};

extern DebugReport g_DbgReporter;

#endif // _INCLUDE_SOURCEMOD_CDBGREPORTER_H_

