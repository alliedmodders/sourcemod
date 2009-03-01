/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Updater Extension
 * Copyright (C) 2004-2009 AlliedModders LLC.  All rights reserved.
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

#ifndef _INCLUDE_SOURCEMOD_UPDATER_H_
#define _INCLUDE_SOURCEMOD_UPDATER_H_

#include <IWebternet.h>
#include <ITextParsers.h>
#include <sh_string.h>
#include "MemoryDownloader.h"

using namespace SourceHook;

struct UpdatePart
{
	UpdatePart* next;
	char *file;
	char *data;
	size_t length;
};

namespace SourceMod
{
	class UpdateReader : 
		public ITextListener_SMC
	{
	public:
		UpdateReader();
		~UpdateReader();
	public: /* ITextListener_SMC */
		void ReadSMC_ParseStart();
		SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name);
		SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value);
		SMCResult ReadSMC_LeavingSection(const SMCStates *states);
	public:
		void PerformUpdate(const char *url);
		UpdatePart *DetachParts();
	private:
		void HandleFile();
		void HandleFolder(const char *folder);
		void LinkPart(UpdatePart *part);
	private:
		IWebTransfer *xfer;
		MemoryDownloader mdl;
		unsigned int ustate;
		unsigned int ignoreLevel;
		SourceHook::String curfile;
		SourceHook::String url;
		char checksum[33];
		UpdatePart *partFirst;
		UpdatePart *partLast;
		const char *update_url;
	};
}

#endif /* _INCLUDE_SOURCEMOD_UPDATER_H_ */

