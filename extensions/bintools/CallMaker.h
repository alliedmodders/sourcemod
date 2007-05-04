/**
 * vim: set ts=4 :
 * ===============================================================
 * SourceMod, Copyright (C) 2004-2007 AlliedModders LLC. 
 * All rights reserved.
 * ===============================================================
 *
 *  This file is part of the SourceMod/SourcePawn SDK.  This file may only be 
 * used or modified under the Terms and Conditions of its License Agreement, 
 * which is found in public/licenses/LICENSE.txt.  As of this notice, derivative 
 * works must be licensed under the GNU General Public License (version 2 or 
 * greater).  A copy of the GPL is included under public/licenses/GPL.txt.
 * 
 * To view the latest information, see: http://www.sourcemod.net/license.php
 *
 * Version: $Id$
 */

#ifndef _INCLUDE_SOURCEMOD_CALLMAKER_H_
#define _INCLUDE_SOURCEMOD_CALLMAKER_H_

#include <IBinTools.h>

using namespace SourceMod;

enum FuncAddrMethod
{
	FuncAddr_Direct,
	FuncAddr_VTable
};

class CallMaker : public IBinTools
{
public: //IBinTools
	ICallWrapper *CreateCall(void *address,
		CallConvention cv,
		const PassInfo *retInfo,
		const PassInfo paramInfo[],
		unsigned int numParams);
	ICallWrapper *CreateVCall(unsigned int vtblIdx,
		unsigned int vtblOffs,
		unsigned int thisOffs,
		const PassInfo *retInfo,
		const PassInfo paramInfo[],
		unsigned int numParams);
};

#endif //_INCLUDE_SOURCEMOD_CALLMAKER_H_
