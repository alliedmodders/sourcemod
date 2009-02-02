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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "extension.h"
#include "Updater.h"
#include <sh_list.h>
#include <sh_string.h>

using namespace SourceHook;

SmUpdater g_Updater;		/**< Global singleton for extension's main interface */
SMEXT_LINK(&g_Updater);

IWebternet *webternet;
static List<String *> update_errors;
static List<String *> update_messages;
static IThreadHandle *update_thread;

bool SmUpdater::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	sharesys->AddDependency(myself, "webternet.ext", true, true);

	SM_GET_IFACE(WEBTERNET, webternet);
	
	ThreadParams params;
	params.flags = Thread_Default;
	params.prio = ThreadPrio_Low;
	update_thread = threader->MakeThread(this, &params);

	if (update_thread == NULL)
	{
		smutils->Format(error, maxlength, "Could not create thread");
		return false;
	}

	return true;
}

void SmUpdater::SDK_OnUnload()
{
	/* An interface drop might have killed this thread.  
	 * But if the handle is still there, we have to wait.
	 */
	if (update_thread != NULL)
	{
		update_thread->WaitForThread();
		update_thread->DestroyThis();
	}

	/* Clear message tables */
	List<String *>::iterator iter;
	while (iter != update_errors.end())
	{
		iter = update_errors.erase(iter);
	}
	while (iter != update_messages.end())
	{
		iter = update_messages.erase(iter);
	}
}

bool SmUpdater::QueryInterfaceDrop(SourceMod::SMInterface *pInterface)
{
	if (pInterface == webternet)
	{
		return false;
	}

	return true;
}

void SmUpdater::NotifyInterfaceDrop(SMInterface *pInterface)
{
	if (pInterface == webternet)
	{
		/* Can't be in the thread if we're losing this extension. */
		update_thread->WaitForThread();
		update_thread->DestroyThis();
		update_thread = NULL;
	}
}

static void LogAllMessages(void *data)
{
	String *str;
	List<String *>::iterator iter;

	if (update_errors.size())
	{
		smutils->LogError(myself, "--- BEGIN ERRORS FROM AUTOMATIC UPDATER ---");

		for (iter = update_errors.begin();
			 iter != update_errors.end();
			 iter++)
		{
			str = (*iter);
			smutils->LogError(myself, "%s", str->c_str());
		}

		smutils->LogError(myself, "--- END ERRORS FROM AUTOMATIC UPDATER ---");
	}

	for (iter = update_messages.begin();
		 iter != update_messages.end();
		 iter++)
	{
		str = (*iter);
		smutils->LogMessage(myself, "%s", str->c_str());
	}
}

void SmUpdater::RunThread(IThreadHandle *pHandle)
{
	UpdateReader ur;

	ur.PerformUpdate();

	if (update_errors.size() || update_messages.size())
	{
		smutils->AddFrameAction(LogAllMessages, NULL);
	}
}

void SmUpdater::OnTerminate(IThreadHandle *pHandle, bool cancel)
{
}

void AddUpdateMessage(const char *fmt, ...)
{
	va_list ap;
	char buffer[2048];

	va_start(ap, fmt);
	smutils->FormatArgs(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	update_messages.push_back(new String(buffer));
}

void AddUpdateError(const char *fmt, ...)
{
	va_list ap;
	char buffer[2048];

	va_start(ap, fmt);
	smutils->FormatArgs(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	update_errors.push_back(new String(buffer));
}
