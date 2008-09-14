/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Extension Code for Metamod:Source
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

#ifndef _INCLUDE_SAMPLE_MMS_SOURCEMOD_EXTENSION_
#define _INCLUDE_SAMPLE_MMS_SOURCEMOD_EXTENSION_

#include "sm_sdk_config.h"

using namespace SourceMod;

class MyExtension : public IExtensionInterface 
{
public:
	virtual bool OnExtensionLoad(IExtension *me,
		IShareSys *sys, 
		char *error, 
		size_t maxlength, 
		bool late);
	virtual void OnExtensionUnload();
	virtual void OnExtensionsAllLoaded();
	virtual void OnExtensionPauseChange(bool pause);
	virtual bool QueryRunning(char *error, size_t maxlength);
	virtual bool IsMetamodExtension();
	virtual const char *GetExtensionName();
	virtual const char *GetExtensionURL();
	virtual const char *GetExtensionTag();
	virtual const char *GetExtensionAuthor();
	virtual const char *GetExtensionVerString();
	virtual const char *GetExtensionDescription();
	virtual const char *GetExtensionDateString();
};

bool SM_LoadExtension(char *error, size_t maxlength);
void SM_UnloadExtension();

extern IShareSys *sharesys;
extern IExtension *myself;
extern MyExtension g_SMExt;

#endif //_INCLUDE_SAMPLE_MMS_SOURCEMOD_EXTENSION_

